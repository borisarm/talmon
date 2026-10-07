#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <utility>

namespace talmon::domain {

enum class ErrorCode {
    Unknown,
    InvalidArgument,
    InvalidConfiguration,
    NotFound,
    Unavailable,
    Unauthenticated,
    PermissionDenied,
    DeadlineExceeded,
    Cancelled,
    Internal,
};

[[nodiscard]] constexpr std::string_view toString(ErrorCode code) noexcept {
    switch (code) {
    case ErrorCode::InvalidArgument: return "InvalidArgument";
    case ErrorCode::InvalidConfiguration: return "InvalidConfiguration";
    case ErrorCode::NotFound: return "NotFound";
    case ErrorCode::Unavailable: return "Unavailable";
    case ErrorCode::Unauthenticated: return "Unauthenticated";
    case ErrorCode::PermissionDenied: return "PermissionDenied";
    case ErrorCode::DeadlineExceeded: return "DeadlineExceeded";
    case ErrorCode::Cancelled: return "Cancelled";
    case ErrorCode::Internal: return "Internal";
    case ErrorCode::Unknown: break;
    }
    return "Unknown";
}

struct Error {
    ErrorCode code{ErrorCode::Unknown};
    std::string message;

    [[nodiscard]] std::string describe() const {
        std::string text{toString(code)};
        if (!message.empty()) {
            text += ": ";
            text += message;
        }
        return text;
    }

    friend bool operator==(const Error&, const Error&) = default;
};

template <typename T>
using Result = std::expected<T, Error>;

[[nodiscard]] inline std::unexpected<Error> fail(ErrorCode code, std::string message) {
    return std::unexpected<Error>{Error{code, std::move(message)}};
}

} // namespace talmon::domain
