// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2018, LH_Mouse. All wrongs reserved.

#ifndef OBJECT_NAME
#  error OBJECT_NAME is undefined.
#endif

#ifndef OBJECT_COLLECTION
#  error OBJECT_COLLECTION is undefined.
#endif

#ifndef OBJECT_FIELDS
#  error OBJECT_FIELDS is undefined.
#endif

#ifndef OBJECT_PRIMARY_KEY
#  error OBJECT_PRIMARY_KEY is undefined.
#endif

#ifndef POSEIDON_MONGODB_OBJECT_BASE_HPP_
#  error Please #include <poseidon/mongodb/object_base.hpp> first.
#endif

class OBJECT_NAME : public ::Poseidon::Mongodb::Object_base {
public:

#undef FIELD_BOOLEAN
#undef FIELD_SIGNED
#undef FIELD_UNSIGNED
#undef FIELD_DOUBLE
#undef FIELD_STRING
#undef FIELD_DATETIME
#undef FIELD_UUID
#undef FIELD_BLOB

#define FIELD_BOOLEAN(id_)                ::Poseidon::Mongodb::Object_base::Field<bool> id_;
#define FIELD_SIGNED(id_)                 ::Poseidon::Mongodb::Object_base::Field< ::boost::int64_t> id_;
#define FIELD_UNSIGNED(id_)               ::Poseidon::Mongodb::Object_base::Field< ::boost::uint64_t> id_;
#define FIELD_DOUBLE(id_)                 ::Poseidon::Mongodb::Object_base::Field<double> id_;
#define FIELD_STRING(id_)                 ::Poseidon::Mongodb::Object_base::Field< ::std::string> id_;
#define FIELD_DATETIME(id_)               ::Poseidon::Mongodb::Object_base::Field< ::boost::uint64_t> id_;
#define FIELD_UUID(id_)                   ::Poseidon::Mongodb::Object_base::Field< ::Poseidon::Uuid> id_;
#define FIELD_BLOB(id_)                   ::Poseidon::Mongodb::Object_base::Field< ::std::basic_string<unsigned char> > id_;

	OBJECT_FIELDS

public:
	OBJECT_NAME();

#undef FIELD_BOOLEAN
#undef FIELD_SIGNED
#undef FIELD_UNSIGNED
#undef FIELD_DOUBLE
#undef FIELD_STRING
#undef FIELD_DATETIME
#undef FIELD_UUID
#undef FIELD_BLOB

#define FIELD_BOOLEAN(id_)                , bool id_##X_
#define FIELD_SIGNED(id_)                 , ::boost::int64_t id_##X_
#define FIELD_UNSIGNED(id_)               , ::boost::uint64_t id_##X_
#define FIELD_DOUBLE(id_)                 , double id_##X_
#define FIELD_STRING(id_)                 , ::std::string id_##X_
#define FIELD_DATETIME(id_)               , ::boost::uint64_t id_##X_
#define FIELD_UUID(id_)                   , const ::Poseidon::Uuid & id_##X_
#define FIELD_BLOB(id_)                   , ::std::basic_string<unsigned char> id_##X_

	explicit OBJECT_NAME(STRIP_FIRST(void OBJECT_FIELDS));

	~OBJECT_NAME() OVERRIDE;

public:
	const char *get_collection() const OVERRIDE;
	void generate_document(::Poseidon::Mongodb::Bson_builder &doc_) const OVERRIDE;
	::std::string generate_primary_key() const OVERRIDE;
	void fetch(const ::boost::shared_ptr<const ::Poseidon::Mongodb::Connection> &conn_) OVERRIDE;
};

#ifdef MONGODB_OBJECT_EMIT_EXTERNAL_DEFINITIONS
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"

OBJECT_NAME::OBJECT_NAME()
	: ::Poseidon::Mongodb::Object_base()

#undef FIELD_BOOLEAN
#undef FIELD_SIGNED
#undef FIELD_UNSIGNED
#undef FIELD_DOUBLE
#undef FIELD_STRING
#undef FIELD_DATETIME
#undef FIELD_UUID
#undef FIELD_BLOB

#define FIELD_BOOLEAN(id_)                , id_(this)
#define FIELD_SIGNED(id_)                 , id_(this)
#define FIELD_UNSIGNED(id_)               , id_(this)
#define FIELD_DOUBLE(id_)                 , id_(this)
#define FIELD_STRING(id_)                 , id_(this)
#define FIELD_DATETIME(id_)               , id_(this)
#define FIELD_UUID(id_)                   , id_(this)
#define FIELD_BLOB(id_)                   , id_(this)

	OBJECT_FIELDS
{
	//
}

#undef FIELD_BOOLEAN
#undef FIELD_SIGNED
#undef FIELD_UNSIGNED
#undef FIELD_DOUBLE
#undef FIELD_STRING
#undef FIELD_DATETIME
#undef FIELD_UUID
#undef FIELD_BLOB

#define FIELD_BOOLEAN(id_)                , bool id_##X_
#define FIELD_SIGNED(id_)                 , ::boost::int64_t id_##X_
#define FIELD_UNSIGNED(id_)               , ::boost::uint64_t id_##X_
#define FIELD_DOUBLE(id_)                 , double id_##X_
#define FIELD_STRING(id_)                 , ::std::string id_##X_
#define FIELD_DATETIME(id_)               , ::boost::uint64_t id_##X_
#define FIELD_UUID(id_)                   , const ::Poseidon::Uuid & id_##X_
#define FIELD_BLOB(id_)                   , ::std::basic_string<unsigned char> id_##X_

OBJECT_NAME::OBJECT_NAME(STRIP_FIRST(void OBJECT_FIELDS))
	: ::Poseidon::Mongodb::Object_base()

#undef FIELD_BOOLEAN
#undef FIELD_SIGNED
#undef FIELD_UNSIGNED
#undef FIELD_DOUBLE
#undef FIELD_STRING
#undef FIELD_DATETIME
#undef FIELD_UUID
#undef FIELD_BLOB

#define FIELD_BOOLEAN(id_)                , id_(this, id_##X_)
#define FIELD_SIGNED(id_)                 , id_(this, id_##X_)
#define FIELD_UNSIGNED(id_)               , id_(this, id_##X_)
#define FIELD_DOUBLE(id_)                 , id_(this, id_##X_)
#define FIELD_STRING(id_)                 , id_(this, STD_MOVE(id_##X_))
#define FIELD_DATETIME(id_)               , id_(this, id_##X_)
#define FIELD_UUID(id_)                   , id_(this, id_##X_)
#define FIELD_BLOB(id_)                   , id_(this, STD_MOVE(id_##X_))

	OBJECT_FIELDS
{
	//
}

OBJECT_NAME::~OBJECT_NAME(){
	//
}

const char *OBJECT_NAME::get_collection() const {
	return OBJECT_COLLECTION;
}
void OBJECT_NAME::generate_document(::Poseidon::Mongodb::Bson_builder &doc_) const {
	PROFILE_ME;

	const ::Poseidon::Recursive_mutex::Unique_lock lock_(m_mutex);

	AUTO(pkey_, generate_primary_key());
	if(!pkey_.empty()){
		doc_.append_string(::Poseidon::Rcnts::view("_id"), STD_MOVE(pkey_));
	}

#undef FIELD_BOOLEAN
#undef FIELD_SIGNED
#undef FIELD_UNSIGNED
#undef FIELD_DOUBLE
#undef FIELD_STRING
#undef FIELD_DATETIME
#undef FIELD_UUID
#undef FIELD_BLOB

#define FIELD_BOOLEAN(id_)                doc_.append_boolean  (::Poseidon::Rcnts::view(TOKEN_TO_STR(id_)), id_);
#define FIELD_SIGNED(id_)                 doc_.append_signed   (::Poseidon::Rcnts::view(TOKEN_TO_STR(id_)), id_);
#define FIELD_UNSIGNED(id_)               doc_.append_unsigned (::Poseidon::Rcnts::view(TOKEN_TO_STR(id_)), id_);
#define FIELD_DOUBLE(id_)                 doc_.append_double   (::Poseidon::Rcnts::view(TOKEN_TO_STR(id_)), id_);
#define FIELD_STRING(id_)                 doc_.append_string   (::Poseidon::Rcnts::view(TOKEN_TO_STR(id_)), id_);
#define FIELD_DATETIME(id_)               doc_.append_datetime (::Poseidon::Rcnts::view(TOKEN_TO_STR(id_)), id_);
#define FIELD_UUID(id_)                   doc_.append_uuid     (::Poseidon::Rcnts::view(TOKEN_TO_STR(id_)), id_);
#define FIELD_BLOB(id_)                   doc_.append_blob     (::Poseidon::Rcnts::view(TOKEN_TO_STR(id_)), id_);

	OBJECT_FIELDS
}
::std::string OBJECT_NAME::generate_primary_key() const {
	PROFILE_ME;

	const ::Poseidon::Recursive_mutex::Unique_lock lock_(m_mutex);

	return OBJECT_PRIMARY_KEY;
}
void fetch(const ::boost::shared_ptr<const ::Poseidon::Mongodb::Connection> &conn_){
	PROFILE_ME;

	const ::Poseidon::Recursive_mutex::Unique_lock lock_(m_mutex);

#undef FIELD_BOOLEAN
#undef FIELD_SIGNED
#undef FIELD_UNSIGNED
#undef FIELD_DOUBLE
#undef FIELD_STRING
#undef FIELD_DATETIME
#undef FIELD_UUID
#undef FIELD_BLOB

#define FIELD_BOOLEAN(id_)                id_.set(conn_->get_boolean  ( TOKEN_TO_STR(id_) ), false);
#define FIELD_SIGNED(id_)                 id_.set(conn_->get_signed   ( TOKEN_TO_STR(id_) ), false);
#define FIELD_UNSIGNED(id_)               id_.set(conn_->get_unsigned ( TOKEN_TO_STR(id_) ), false);
#define FIELD_DOUBLE(id_)                 id_.set(conn_->get_double   ( TOKEN_TO_STR(id_) ), false);
#define FIELD_STRING(id_)                 id_.set(conn_->get_string   ( TOKEN_TO_STR(id_) ), false);
#define FIELD_DATETIME(id_)               id_.set(conn_->get_datetime ( TOKEN_TO_STR(id_) ), false);
#define FIELD_UUID(id_)                   id_.set(conn_->get_uuid     ( TOKEN_TO_STR(id_) ), false);
#define FIELD_BLOB(id_)                   id_.set(conn_->get_blob     ( TOKEN_TO_STR(id_) ), false);

	OBJECT_FIELDS
}

#pragma GCC diagnostic pop
#endif // MONGODB_OBJECT_EMIT_EXTERNAL_DEFINITIONS

#undef OBJECT_NAME
#undef OBJECT_FIELDS
#undef OBJECT_PRIMARY_KEY