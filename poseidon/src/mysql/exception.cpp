// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2018, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "exception.hpp"
#include "../log.hpp"

namespace Poseidon {
namespace Mysql {

Exception::Exception(const char *file, std::size_t line, const char *func, Rcnts schema, unsigned long code, Rcnts message)
	: Basic_exception(file, line, func, STD_MOVE(message))
	, m_schema(STD_MOVE(schema)), m_code(code)
{
	POSEIDON_LOG_ERROR("Mysql::Exception: schema = ", get_schema(), ", code = ", get_code(), ", what = ", what());
}
Exception::~Exception() NOEXCEPT {
	//
}

}
}
