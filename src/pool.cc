// Storage pools.

#include "pool.hh"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/regex.hpp>

namespace bf = boost::filesystem;
namespace bu = boost::uuids;
namespace po = boost::program_options;

// This can be made std once that works in all toolchains.
namespace re = boost;

namespace cdump {

namespace {

std::string make_uuid() {
  bu::random_generator gen;
  bu::uuid id = gen();

  return bu::to_string(id);
}

// Ensure the specified name is a directory an it is empty.
void ensure_empty(const std::string path) {
  if (!bf::is_directory(path))
    throw std::invalid_argument("path doesn't name directory");
  if (bf::directory_iterator(path) != bf::directory_iterator())
    throw std::invalid_argument("path doesn't name empty directory");
}

} // namespace

// Pool construction.  Ensures the directory is completely blank, and
// writes the properties file to it.
void Pool::create_pool(const std::string path,
		       unsigned limit,
		       bool newlib)
{
  if (limit < limit_lower_bound || limit >= limit_upper_bound)
    throw std::invalid_argument("limit out of range");
  ensure_empty(path);

  bf::path metadata(path);
  metadata /= "metadata";
  if (!bf::create_directory(metadata))
    throw std::runtime_error("Unable to create metadata directory");

  {
    bf::path props(metadata);
    props /= "props.txt";
    std::ofstream out(props.string());
    out << "# Ldump metadata properties (odump)\n"
	<< "uuid=" << make_uuid() << "\n"
	<< "newfile=" << std::boolalpha << newlib << "\n"
	<< "limit=" << limit << "\n";
  }
}

void Pool::read_props(std::string path) {
  po::options_description desc("Properties file");

  // std::string uuid;
  props.uuid = bu::nil_uuid();
  props.newfile = false;
  props.limit = Pool::default_limit;

  desc.add_options()
      ("uuid", po::value<bu::uuid>(&props.uuid), "uuid")
      ("newfile", po::value<bool>(&props.newfile), "newfile")
      ("limit", po::value<unsigned>(&props.limit), "limit");

  po::variables_map vm;
  po::store(po::parse_config_file<char>(path.c_str(), desc), vm);
  po::notify(vm);

  // It seems that program_options fails anyway if the uuid isn't
  // specified, perhaps because it doesn't have a default constructor.
  if (props.uuid.is_nil())
    throw new std::runtime_error("Backup property file \"" + path +
				 "\" doesn't contain uuid");

  // std::cout << bu::to_string(props.uuid) << std::endl;
  // std::cout << props.limit << std::endl;
}

std::vector<OID> Pool::get_backups() const {
  bf::path backups = base;
  backups /= "metadata";
  backups /= "backups.txt";

  std::vector<OID> result;
  std::string line;

  std::ifstream inp(backups.string());

  while (!inp.eof()) {
    inp >> line;
    if (inp.fail())
      break;
    result.emplace_back(line);
  }

  return result;
}

Pool::Pool(const std::string path, bool writable)
  : base(path), writable(writable),
    lock(lock_path().c_str())
{
  bf::path ppath(base);
  ppath /= "metadata";
  ppath /= "props.txt";
  read_props(ppath.string());
  first_newfile = props.newfile;
  scan_files();
}

std::string Pool::lock_path() {
  auto work = base;
  work /= "lock";
  return work.string();
}

ChunkPtr Pool::find(const OID& key) {
  for (auto& f : files) {
    const auto res = f.index.find(key);
    if (res != f.index.end()) {
      f.file.seekg(res->second.offset);
      return Chunk::read(f.file);
    }
  }

  return ChunkPtr();
}

void Pool::prepare_write(unsigned size) {
  bool force_new = first_newfile;

  // Decide if we need to open a new file, or the existing one.
  if (dirty) {
    // If there is room, just write.
    if (write_pos + size <= props.limit)
      return;

    // Otherwise, flush, and move on to a new file.
    flush();
    force_new = true;
  } else {
    // If there is a file, see if there would be room to write to it.
    if (files.empty())
      force_new = true;
    else {
      if (files.front().size + size > props.limit)
	force_new = true;
    }
  }

  // At this point, force_new tells us if we are creating a new file,
  // or opening the last one.
  if (force_new) {
    unsigned index = 0;
    if (!files.empty())
      index = files.front().pos + 1;
    files.emplace_front(*this, index, true);
  } else {
    files.front().make_writable(*this);
  }

  dirty = true;
  first_newfile = false;
}

void Pool::insert(ChunkPtr chunk) {
  if (!writable)
    throw std::logic_error("Attempt to insert into class opened as read-only");

  prepare_write(chunk->write_size());

  auto& file = files.front();

  // It seems that fstream doesn't properly handle tellp() if the
  // position isn't sought first.  It would be more efficient to not
  // do this, and perhaps that could be done without the check below.
  // The danger is losing track.
  file.file.seekp(0, std::ios::end);
  // std::cout << std::hex;
  // std::cout << "--------------\n";
  // std::cout << "Pre : " << file.size << std::endl;
  // std::cout << "PPos: " << file.file.tellp() << std::endl;
  chunk->write(file.file);
  file.index.insert(FileIndex::value_type(chunk->get_oid(),
					  FileIndex::Node(file.size, chunk->get_kind())));
  file.size += chunk->write_size();
  // std::cout << "Wsiz: " << chunk->write_size() << std::endl;
  // std::cout << "Size: " << file.size << std::endl;
  // std::cout << " Pos: " << file.file.tellp() << std::endl;
  // std::cout << std::dec;
  if (file.size != file.file.tellp()) {
    throw std::runtime_error("File position mismatch on write");
  }
}

void Pool::flush() {
  if (dirty) {
    auto& file = files.front();
    file.unmake_writable(*this);
    const auto name = construct_name(file.pos, ".idx");
    files.front().index.save(name, file.size);
    files.front().index.load(name, file.size);

    dirty = false;
  }
}

namespace {
// Attempt to decode the given filename to determine if it is a pool
// data file.  These files are of the form "pool-data-nnnn.data",
// where 'n' is a sequence of digits.  Returns -1 if the name
// does not represent a numbered file, or a non-negative integer if it
// does.
int decode_name(const std::string name) {
  static const re::regex pattern (R"(pool-data-(\d{4})\.data)");
  re::smatch result;
  if (re::regex_match(name, result, pattern)) {
    return std::stoi(result.str(1));
  }
  return -1;
}
}

void Pool::scan_files() {
  std::vector<unsigned> known;

  for (auto elt = bf::directory_iterator(base);
       elt != bf::directory_iterator();
       ++elt)
  {
    const auto name = elt->path().filename().string();
    const auto pos = decode_name(name);
    if (pos >= 0)
      known.push_back(pos);
  }

  std::sort(known.begin(), known.end());

  // Open each of the files.
  for (auto elt : known) {
    files.emplace_front(*this, elt);
  }

  // std::copy(known.begin(), known.end(),
  //           std::ostream_iterator<unsigned>(std::cout, ", "));
  // std::cout << std::endl;
}

/**
 * Construct the filename for the given file.
 *
 * @param pos the file number.  Will expand to the 4 digit number
 * within the name
 * @param extension the file extension to use, including the leading
 * dot.
 */
std::string Pool::construct_name(unsigned pos, const std::string extension) const {
  auto work = base;
  std::ostringstream name;
  name << "pool-data-" << std::setfill('0') << std::setw(4) << pos
      << extension;
  work /= name.str();
  return work.string();
}

namespace {
std::ios::openmode determine_mode(bool create) {
  std::ios::openmode result = std::ios::binary | std::ios::in;
  if (create)
    result |= std::ios::out | std::ios::app;
  return result;
}
}

Pool::File::File(const Pool& parent, unsigned pos, bool create)
  : pos(pos),
    file(parent.construct_name(pos, ".data"),
	 determine_mode(create))
{
  // std::cout << "file: " << parent.construct_name(pos, ".data") << std::endl;
  // std::cout << "mode: " << determine_mode(create) << std::endl;
  if (!file.is_open())
    throw std::runtime_error("Unable to open pool file");
  file.seekg(0, std::ios::end);
  size = file.tellg();
  if (!create)
    index.load(parent.construct_name(pos, ".idx"), size);
}

void Pool::File::make_writable(const Pool& parent) {
  file.close();
  file.open(parent.construct_name(pos, ".data"),
	    determine_mode(true));
}

void Pool::File::unmake_writable(const Pool& parent) {
  file.close();
  file.open(parent.construct_name(pos, ".data"),
	    determine_mode(false));
}

} // namespace cdump
