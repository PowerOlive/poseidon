// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2018, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_MYSQL_CONNECTION_HPP_
#define POSEIDON_MYSQL_CONNECTION_HPP_

#include "../cxx_ver.hpp"
#include "../cxx_util.hpp"
#include "../fwd.hpp"
#include <string>
#include <cstring>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

namespace Poseidon {
namespace MySql {

class Connection : NONCOPYABLE {
public:
	static boost::shared_ptr<Connection> create(const char *server_addr, boost::uint16_t server_port, const char *user_name, const char *password, const char *schema, bool use_ssl, const char *charset);

public:
	virtual ~Connection() = 0;

public:
	void execute_sql(const char *sql, std::size_t len);
	void execute_sql(const char *sql){
		execute_sql(sql, std::strlen(sql));
	}
	void execute_sql(const std::string &sql){
		execute_sql(sql.data(), sql.size());
	}
	void discard_result() NOEXCEPT;

	boost::uint64_t get_insert_id() const;
	bool fetch_row();

	boost::int64_t get_signed(const char *name) const;
	boost::uint64_t get_unsigned(const char *name) const;
	double get_double(const char *name) const;
	std::string get_string(const char *name) const;
	boost::uint64_t get_datetime(const char *name) const;
	Uuid get_uuid(const char *name) const;
	std::basic_string<unsigned char> get_blob(const char *name) const;
};

}
}

#endif
