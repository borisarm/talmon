#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <string_view>
#include <vector>

namespace talmon::application {

/// Turns an arbitrary chunked byte stream into complete lines.
class LineAssembler {
public:
    /// Returns the lines completed by `chunk` (without the trailing '\n' / "\r\n").
    [[nodiscard]] std::vector<std::string> feed(std::string_view chunk);

    /// Returns the pending partial line, if any, and clears it.
    [[nodiscard]] std::vector<std::string> flush();

private:
    std::string pending_;
};

/// Keeps the last `capacity` lines (ring buffer semantics).
class BoundedLineBuffer {
public:
    explicit BoundedLineBuffer(std::size_t capacity);

    /// Returns how many old lines were evicted.
    std::size_t append(std::vector<std::string> lines);
    void clear() noexcept { lines_.clear(); }

    [[nodiscard]] std::size_t size() const noexcept { return lines_.size(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] const std::deque<std::string>& lines() const noexcept { return lines_; }
    [[nodiscard]] std::string joined() const;

private:
    std::size_t capacity_;
    std::deque<std::string> lines_;
};

} // namespace talmon::application
