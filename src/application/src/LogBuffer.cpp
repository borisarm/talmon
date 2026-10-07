#include "talmon/application/LogBuffer.hpp"

#include <algorithm>

namespace talmon::application {

namespace {

std::string trimCarriageReturn(std::string line) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return line;
}

} // namespace

std::vector<std::string> LineAssembler::feed(std::string_view chunk) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (true) {
        const auto newline = chunk.find('\n', start);
        if (newline == std::string_view::npos) {
            pending_.append(chunk.substr(start));
            break;
        }
        pending_.append(chunk.substr(start, newline - start));
        lines.push_back(trimCarriageReturn(std::move(pending_)));
        pending_.clear();
        start = newline + 1;
    }
    return lines;
}

std::vector<std::string> LineAssembler::flush() {
    std::vector<std::string> lines;
    if (!pending_.empty()) {
        lines.push_back(trimCarriageReturn(std::move(pending_)));
        pending_.clear();
    }
    return lines;
}

BoundedLineBuffer::BoundedLineBuffer(std::size_t capacity) : capacity_{std::max<std::size_t>(capacity, 1)} {}

std::size_t BoundedLineBuffer::append(std::vector<std::string> lines) {
    for (auto& line : lines) {
        lines_.push_back(std::move(line));
    }
    std::size_t evicted = 0;
    while (lines_.size() > capacity_) {
        lines_.pop_front();
        ++evicted;
    }
    return evicted;
}

std::string BoundedLineBuffer::joined() const {
    std::string text;
    for (const auto& line : lines_) {
        text += line;
        text += '\n';
    }
    return text;
}

} // namespace talmon::application
