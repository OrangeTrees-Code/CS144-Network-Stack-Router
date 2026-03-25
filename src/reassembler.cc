#include "reassembler.hh"
#include "debug.hh"
// #include <string>

using namespace std;

void Reassembler::TruncateIfTooLong(uint64_t first_index, string& data)
{

  uint64_t window_end = next_index + output_.writer().available_capacity();

  if (first_index > window_end) {
    data.clear();
    return;
  }
  if (first_index + data.length() > window_end) {
    data = data.substr(0, window_end - first_index);
  }
}

string mergeStr(string& data,
  uint64_t data_start,
  string& orig,
  uint64_t orig_start,
  pair<uint64_t, uint64_t>* bounds)
{
  bounds->first = min(data_start, orig_start);
  bounds->second = max(data_start + data.length(), orig_start + orig.length());

  uint64_t& start = bounds->first;
  uint64_t& end = bounds->second;

  string res(end - start, '\0');
  for (size_t i = 0; i < orig.length(); ++i) {
    res[orig_start - start + i] = orig[i];
  }

  for (size_t i = 0; i < data.length(); ++i) {
    res[data_start - start + i] = data[i];
  }

  return res;
}

string Reassembler::doMerge(Reassembler::SegMapIter& it,
  uint64_t first_index,
  string& data,
  pair<uint64_t, uint64_t>* bounds)
{
  pair<uint64_t, uint64_t> orig_bounds = it->first;
  string orig = it->second;
  it = segs.erase(it);

  string merged = mergeStr(data, first_index, orig, orig_bounds.first, bounds);

  return merged;
}

void Reassembler::insertAndTryMerge(uint64_t first_index, string& data)
{
  pair<uint64_t, uint64_t> bounds;
  bounds.first = first_index;
  bounds.second = first_index + data.length();
  string content = data;

  auto it = segs.lower_bound(bounds);
  if (it != segs.begin()) {
    auto prev = std::prev(it);
    auto [low, high] = prev->first;
    if (high >= bounds.first) {
      content = doMerge(prev, bounds.first, content, &bounds);
    }
  }

  while (it != segs.end() && it->first.first <= bounds.second) {
    content = doMerge(it, bounds.first, content, &bounds);
  }

  segs.insert({ bounds, content });
}

void Reassembler::insert(uint64_t first_index, string data, bool is_last_substring)
{
  if (is_last_substring == true) {
    eof_index = first_index + data.length();
  }

  if (data.empty()) {
    if (is_last_substring) {
      output_.writer().close();
    }
    return;
  }

  if (first_index + data.size() <= next_index)
    return;

  TruncateIfTooLong(first_index, data);
  if (data.empty() == true) {
    return;
  }

  // debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
  insertAndTryMerge(first_index, data);

  while (segs.empty() == false) {
    auto it = segs.begin();
    auto [bounds, str] = *it;
    if (next_index < bounds.first) {
      break;
    }

    if (next_index > bounds.first) {
      str = str.substr(next_index - bounds.first);
    }

    output_.writer().push(str);
    next_index += str.length();
    segs.erase(it);
  }

  if (eof_index.has_value() && next_index == eof_index.value()) {
    output_.writer().close();
    return;
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t bytes = 0;
  for (auto [bounds, str] : segs) {
    bytes += str.length();
  }
  return bytes;
}
