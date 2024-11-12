﻿/*
	Copyright (c) 2017 TOSHIBA Digital Solutions Corporation
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*!
	@file
	@brief Definition of sql common
*/
#ifndef SQL_COMMON_H_
#define SQL_COMMON_H_
#include "config_table.h"




const int32_t TABLE_PARTITIONING_MAX_HASH_PARTITIONING_NUM = 1024;
const int32_t TABLE_PARTITIONING_MIN_INTERVAL_VALUE = 1000;
const int32_t TABLE_PARTITIONING_DEFAULT_MAX_ASSIGN_NUM = 10000;
const int32_t TABLE_PARTITIONING_MAX_ASSIGN_ENTRY_NUM = 1000000;
const int32_t TABLE_PARTITIONING_LIMIT_MAX_ASSIGN_NUM = TABLE_PARTITIONING_MAX_ASSIGN_ENTRY_NUM;

typedef int8_t LargeContainerStatusType;

const LargeContainerStatusType PARTITION_STATUS_CREATE_START = 0;
const LargeContainerStatusType PARTITION_STATUS_CREATE_END = 1;
const LargeContainerStatusType PARTITION_STATUS_DROP_START = 2;
const LargeContainerStatusType PARTITION_STATUS_DROP_END = 3;
const LargeContainerStatusType INDEX_STATUS_CREATE_START = 4;
const LargeContainerStatusType INDEX_STATUS_CREATE_END = 5;
const LargeContainerStatusType INDEX_STATUS_DROP_START = 6;
const LargeContainerStatusType INDEX_STATUS_DROP_END = 7;
const LargeContainerStatusType TABLE_STATUS_DROP_START = 8;
const LargeContainerStatusType PARTITION_STATUS_NONE = -1;

typedef int8_t ContainerCategory;
const LargeContainerStatusType LARGE_CONTAINER_STATUS = 0;
const LargeContainerStatusType TABLE_PARTITIONING_VERSION = 1;
const LargeContainerStatusType TABLE_PARTITIONING_RECOVERY = 2;
const LargeContainerStatusType TABLE_PARTITIONING_CHECK_EXPIRED = 3;
const LargeContainerStatusType UNDEF_LARGE_CONTAINER_CATEGORY = -1;

static const int32_t DEFAULT_NOSQL_FAILOVER_WAIT_TIME = 3 * 1000;

typedef util::VariableSizeAllocator<> SQLVariableSizeLocalAllocator;
typedef util::VariableSizeAllocatorTraits<> SQLVariableSizeGlobalAllocatorTraits;
typedef util::VariableSizeAllocator<
	util::Mutex, SQLVariableSizeGlobalAllocatorTraits> SQLVariableSizeGlobalAllocator;

typedef util::BasicString<
	char8_t, std::char_traits<char8_t>,
	util::StdAllocator<char8_t, SQLVariableSizeGlobalAllocator> > SQLString;

typedef int64_t NoSQLSyncId;
const int64_t UNDEF_NOSQL_SYNCID = -1;

typedef int64_t NoSQLRequestId;
const int64_t UNDEF_NOSQL_REQUESTID = -1;

enum ExpirationType {
	EXPIRATION_TYPE_ROW = 2,
	EXPIRATION_TYPE_PARTITION
};

enum SQLErrorCode {
	GS_ERROR_SQL_SERVICE_START_FAILED = 200000,
	GS_ERROR_SQL_SERVICE_NOT_INITIALIZED,
	GS_ERROR_SQL_ENCODE_FAILED,
	GS_ERROR_SQL_INVALID_SENDER_ND,
	GS_ERROR_SQL_OPTYPE_UNSUPPORTED,
	GS_ERROR_SQL_OPTYPE_UNKNOWN,
	GS_ERROR_SQL_COLLECT_TIMEOUT_CONTEXT_FAILED,
	GS_ERROR_SQL_CANCELLED,
	GS_ERROR_SQL_RETRY_LIMIT_EXCEEDED,
	GS_ERROR_SQL_VALUETYPE_UNSUPPORTED,
	GS_ERROR_SQL_BIND_PARAMETER_FAILED,
	GS_ERROR_SQL_CANCELED,
	GS_ERROR_SQL_INVALID_AUTHORIZATION,
	GS_ERROR_SQL_INVALID_USER,
	GS_ERROR_SQL_INVALID_NAME,
	GS_ERROR_SQL_DML_FAILED,
	GS_ERROR_SQL_DDL_FAILED,
	GS_ERROR_SQL_INTERNAL,
	GS_ERROR_SQL_UNSUPPORTED,
	GS_ERROR_SQL_CONNECTION_ALREADY_CLOSED,
	GS_ERROR_SQL_STATEMENT_ALREADY_CLOSED,
	GS_ERROR_SQL_CONTAINER_NOT_FOUND,
	GS_ERROR_SQL_INVALID_DATABASE,
	GS_ERROR_SQL_TABLE_ALREADY_EXIST,
	GS_ERROR_SQL_STATEMENT_CATEGORY_UNMATCHED,
	GS_ERROR_SQL_FETCH_SIZE_INVALID,
	GS_ERROR_SQL_INACCESSIBLE_TABLE,
	GS_ERROR_SQL_CLUSTER_FAILOVER,
	GS_ERROR_SQL_TYPE_MISMATCH,
	GS_ERROR_SQL_ILLEGAL_TABLE_INFO_DETECTED,
	GS_ERROR_SQL_PARTITION_STATE_UNDEFINED,
	GS_ERROR_SQL_STATEMENT_NOT_FOUND,
	GS_ERROR_SQL_INVALID_QUERY_EXECUTION_ID,
	GS_ERROR_SQL_CLIENT_REQUEST_PROTOCOL_ERROR,
	GS_ERROR_SQL_INVALID_SYNTAX_TREE,
	GS_ERROR_SQL_INVALID_SCHEMA_INFO,
	GS_ERROR_SQL_CLIENT_FAILOVER_FAILED,
	GS_ERROR_SQL_MSG_VERSION_NOT_ACCEPTABLE,
	GS_ERROR_SQL_INVALID_CLUSTER_STATUS,
	GS_ERROR_SQL_TOTAL_MEMORY_EXCEEDED,
	GS_ERROR_SQL_CONFIG_INVALID,

	GS_ERROR_SQL_MODULE_CREATE_FAILED = 210000,
	GS_ERROR_SQL_CONTEXT_PUT_FAILED,
	GS_ERROR_SQL_CONTEXT_GET_FAILED,
	GS_ERROR_SQL_CONTEXT_REMOVE_FAILED,
	GS_ERROR_SQL_CONTEXT_NOT_FOUND,
	GS_ERROR_SQL_CONTEXT_CREATION_MODE_INVALID,
	GS_ERROR_SQL_CONTEXT_TIMEOUT_CHECK_FAILED,

	GS_ERROR_SQL_TABLE_CREATE_FAILED = 220000,
	GS_ERROR_SQL_TABLE_GET_FAILED,
	GS_ERROR_SQL_TABLE_DROP_FAILED,
	GS_ERROR_SQL_TABLE_APPEND_VALUE_FAILED,
	GS_ERROR_SQL_TABLE_GET_VALUE_FAILED,
	GS_ERROR_SQL_TABLE_IMPORT_FAILED,
	GS_ERROR_SQL_TABLE_EXPORT_FAILED,
	GS_ERROR_SQL_TABLE_SCHEMA_INVALID,
	GS_ERROR_SQL_TABLE_CLEAR_FAILED,

	GS_ERROR_SQL_TQL_CREATE_FAILED = 230000,
	GS_ERROR_SQL_TQL_EXECUTE_FAILED,
	GS_ERROR_SQL_TQL_CONTAINER_NOT_FOUND,
	GS_ERROR_SQL_TQL_PUT_ROW_FAILED,
	GS_ERROR_SQL_TQL_SCHEMA_UNMATCH,
	GS_ERROR_SQL_TQL_EXECUTE_PRAGMA_FAILED,
	GS_ERROR_SQL_TQL_EXECUTE_DIRECT_RESULT_FAILED,
	GS_ERROR_SQL_TQL_EXECUTE_DIRECT_FAILED,
	GS_ERROR_SQL_TQL_CONTAINER_CHANGED,
	GS_ERROR_SQL_TQL_RESULT_TABLE_NOT_FOUND,

	GS_ERROR_SQL_COMPILE_UNSUPPORTED = 240000,
	GS_ERROR_SQL_COMPILE_SYNTAX_ERROR,
	GS_ERROR_SQL_COMPILE_INVALID_SCHEMA,
	GS_ERROR_SQL_COMPILE_INACCESSIBLE_DATABASE,
	GS_ERROR_SQL_COMPILE_INTERNAL,
	GS_ERROR_SQL_COMPILE_INVALID_ARG_COUNT,
	GS_ERROR_SQL_COMPILE_DATABASE_NOT_FOUND,
	GS_ERROR_SQL_COMPILE_TABLE_NOT_FOUND,
	GS_ERROR_SQL_COMPILE_COLUMN_NOT_FOUND,
	GS_ERROR_SQL_COMPILE_COLUMN_NOT_RESOLVED,
	GS_ERROR_SQL_COMPILE_FUNCTION_NOT_FOUND,
	GS_ERROR_SQL_COMPILE_COLUMN_LIST_UNMATCH,
	GS_ERROR_SQL_COMPILE_INVALID_NODE_ASSIGN,
	GS_ERROR_SQL_COMPILE_INVALID_CONTAINER_ATTRIBUTE,
	GS_ERROR_SQL_COMPILE_MISMATCH_SCHEMA,
	GS_ERROR_SQL_COMPILE_INVALID_INSERT_COLUMN,
	GS_ERROR_SQL_COMPILE_PARTITIONING_KEY_NOT_UPDATABLE,
	GS_ERROR_SQL_COMPILE_LIMIT_EXCEEDED,
	GS_ERROR_SQL_COMPILE_PRIMARY_KEY_NOT_UPDATABLE,

	GS_ERROR_SQL_EXECUTION_INTERNAL = 250000,
	GS_ERROR_SQL_EXECUTION_INTERNAL_NOT_FOUND,
	GS_ERROR_SQL_EXECUTION_INTERNAL_ALREADY_EXIST,
	GS_ERROR_SQL_EXECUTION_INVALID_JOB_STATUS,
	GS_ERROR_SQL_EXECUTION_INVALID_STATEMENT_ID,
	GS_ERROR_SQL_EXECUTION_INVALID_STATUS,
	GS_ERROR_SQL_EXECUTION_BIND_FAILED,
	GS_ERROR_SQL_EXECUTION_RETRY_MAX,
	GS_ERROR_SQL_EXECUTION_SCHEMA_MISMATCH,
	GS_ERROR_SQL_EXECUTION_INVALID_FETCH_STATEMENT,
	GS_ERROR_SQL_EXECUTION_INTERNAL_RETRY_MAX,
	GS_ERROR_SQL_COMPILE_METATABLE_NOT_UPDATABLE,
	GS_ERROR_SQL_EXECUTION_ALREADY_SEND_RESULTSET,
	GS_ERROR_SQL_EXECUTION_RETRY_QUERY_LIMIT,
	GS_ERROR_SQL_DENY_REQUEST,
	GS_ERROR_SQL_ADD_BATCH_LIMIT_EXCEEDED,
	GS_ERROR_SQL_INVALID_BATCH_STATEMENT,

	GS_ERROR_JOB_MANAGER_INTERNAL = 260000,
	GS_ERROR_JOB_MANAGER_INVALID_PROTOCOL,
	GS_ERROR_JOB_UNSUPPORTED,
	GS_ERROR_JOB_INITIALIZE_FAILED,
	GS_ERROR_JOB_INTERNAL,
	GS_ERROR_JOB_ALREADY_EXISTS,
	GS_ERROR_JOB_NOT_FOUND,
	GS_ERROR_JOB_RESOLVE_NODE_FAILED,
	GS_ERROR_JOB_INVALID_JOBINFO,
	GS_ERROR_JOB_INVALID_STATUS,
	GS_ERROR_JOB_INVALID_SETTING_PROCESSOR,
	GS_ERROR_JOB_INVALID_CONTROL_TYPE,
	GS_ERROR_JOB_INVALID_SERVICE_TYPE,
	GS_ERROR_JOB_INVALID_CONTROL_INFO,
	GS_ERROR_JOB_CANCELLED,
	GS_ERROR_JOB_INVALID_RECV_BLOCK_NO,
	GS_ERROR_JOB_INVALID_JOBID_FORMAT,
	GS_ERROR_JOB_REQUEST_CANCEL,
	GS_ERROR_JOB_TIMEOUT,
	GS_ERROR_JOB_NODE_FAILURE,

	GS_ERROR_SQL_DML_INTERNAL = 270000,
	GS_ERROR_SQL_DML_UPDATE_PARTITIONING_KEY_CHANGE,
	GS_ERROR_SQL_DML_DELETE_INVALID_SCHEMA,
	GS_ERROR_SQL_DML_INVALID_CONTAINER_ATTRIBUTE,
	GS_ERROR_SQL_DML_EXPIRED_SUB_CONTAINER_VERSION,
	GS_ERROR_SQL_NODE_FAILURE,
	GS_ERROR_SQL_DML_INSERT_INVALID_NULL_CONSTRAINT,
	GS_ERROR_SQL_DML_CONTAINER_STAT_GAP_TOO_LARGE,

	GS_ERROR_SQL_DDL_INTERNAL = 280000,
	GS_ERROR_SQL_DDL_INVALID_USER,
	GS_ERROR_SQL_DDL_UNSUPPORTED_COMMAND_TYPE,
	GS_ERROR_SQL_DDL_TABLE_ALREADY_EXISTS,
	GS_ERROR_SQL_DDL_DATABASE_NOT_EMPTY,
	GS_ERROR_SQL_DDL_TABLE_NOT_EXISTS,
	GS_ERROR_SQL_DDL_INACCESSIBLE_TABLE,
	GS_ERROR_SQL_DDL_INDEX_ALREADY_EXISTS,
	GS_ERROR_SQL_DDL_INDEX_NOT_EXISTS,
	GS_ERROR_SQL_DDL_INDEX_INVALID_COLUMN,
	GS_ERROR_SQL_DDL_INVALID_CONTAINER_ATTRIBUTE,
	GS_ERROR_SQL_DDL_INVALID_CONNECTED_DATABASE,
	GS_ERROR_SQL_DDL_INVALID_PARAMETER,
	GS_ERROR_SQL_DDL_INVALID_PARTITION_COLUMN,
	GS_ERROR_SQL_DDL_INVALID_PRIMARY_KEY,
	GS_ERROR_SQL_DDL_INVALID_PARTITION_COUNT,
	GS_ERROR_SQL_DDL_INVALID_COLUMN,
	GS_ERROR_SQL_INVALID_SCHEMA_CACHE,
	GS_ERROR_SQL_UNSUPPORTED_TABLE_FORMAT_VERSION,
	GS_ERROR_SQL_INVALID_TABLE_FORMAT,
	GS_ERROR_SQL_INVALID_SUB_CONTAINER_EXISTS,
	GS_ERROR_SQL_DDL_ALREADY_RUNNING,
	GS_ERROR_SQL_DDL_UNSUPPORTED_PARTITIONING_INFO,
	GS_ERROR_SQL_DDL_INVALID_PARTITIONING_TABLE_NAME,
	GS_ERROR_SQL_DDL_INVALID_PARTITIONING_TABLE_TYPE,
	GS_ERROR_SQL_TABLE_PARTITION_ALREADY_REMOVED,
	GS_ERROR_SQL_TABLE_PARTITION_MAX_ASSIGN_COUNT,
	GS_ERROR_SQL_TABLE_PARTITION_INTERNAL,
	GS_ERROR_SQL_TABLE_PARTITION_INVALID_MESSAGE,
	GS_ERROR_SQL_TABLE_PARTITION_PARTITIONING_TABLE_NOT_FOUND,
	GS_ERROR_SQL_DDL_CREATE_TABLE_RECOVERY,
	GS_ERROR_SQL_TABLE_PARTITION_LARGE_CONTAINER_INVALID_STATE_CHANGE,
	GS_ERROR_SQL_TABLE_PARTITION_SCHEMA_UNMATCH,
	GS_ERROR_SQL_TABLE_PARTITION_INVALID_INDEX,
	GS_ERROR_SQL_DDL_TABLE_ALREADY_EXISTS_WITH_EXECUTION,
	GS_ERROR_SQL_DDL_INDEX_ALREADY_EXISTS_WITH_EXECUTION,
	GS_ERROR_SQL_DDL_MISMATCH_CONTAINER_ATTRIBUTE,
	GS_ERROR_SQL_DDL_VIEW_NOT_EXISTS,

	GS_ERROR_SQL_PROC_BASE = 300000,

	GS_ERROR_SQL_PROC_INTERNAL = 305000,
	GS_ERROR_SQL_PROC_INTERNAL_INVALID_OPTION,
	GS_ERROR_SQL_PROC_INTERNAL_INVALID_EXPRESSION,
	GS_ERROR_SQL_PROC_INTERNAL_INVALID_INPUT,
	GS_ERROR_SQL_PROC_VALUE_OVERFLOW,
	GS_ERROR_SQL_PROC_VALUE_SYNTAX_ERROR,
	GS_ERROR_SQL_PROC_DIVIDE_BY_ZERO,
	GS_ERROR_SQL_PROC_LIMIT_EXCEEDED,
	GS_ERROR_SQL_PROC_INVALID_EXPRESSION_INPUT,
	GS_ERROR_SQL_PROC_MULTIPLE_TUPLE,
	GS_ERROR_SQL_PROC_UNSUPPORTED_TYPE_CONVERSION,
	GS_ERROR_SQL_PROC_RESULT_FAILED,
	GS_ERROR_SQL_PROC_INVALID_CONSTRAINT_NULL,
	GS_ERROR_SQL_PROC_INTERNAL_INDEX_UNMATCH,

	GS_ERROR_LTS_UNSUPPORTED = 410000,
	GS_ERROR_LTS_NO_MEMORY,
	GS_ERROR_LTS_NOT_IMPLEMENTED,
	GS_ERROR_LTS_INVALID_GROUP_ID,
	GS_ERROR_LTS_INVALID_RESOURCE_ID,
	GS_ERROR_LTS_INVALID_RESOURCE_BLOCK_ID,
	GS_ERROR_LTS_INVALID_DIRECTORY,
	GS_ERROR_LTS_INVALID_LOGICAL_FILE,
	GS_ERROR_LTS_INVALID_PARAMETER,
	GS_ERROR_LTS_TUPLELIST_BLOCKREADER_CREATE_FAILED,
	GS_ERROR_LTS_TUPLELIST_BLOCKWRITER_CREATE_FAILED,
	GS_ERROR_LTS_TUPLELIST_READER_CREATE_FAILED,
	GS_ERROR_LTS_TUPLELIST_WRITER_CREATE_FAILED,
	GS_ERROR_LTS_TUPLELIST_IS_NOT_EXIST,
	GS_ERROR_LTS_TUPLELIST_OPERATION_NOT_SUPPORTED,
	GS_ERROR_LTS_TUPLELIST_BLOCK_APPEND_FAILED,
	GS_ERROR_LTS_SETTING_CAN_NOT_CHANGE,
	GS_ERROR_LTS_TUPLEVALUE_INDEX_OUT_OF_RANGE,
	GS_ERROR_LTS_TYPE_NOT_MATCH,
	GS_ERROR_LTS_TUPLELIST_IS_NOT_SAME,
	GS_ERROR_LTS_READER_NOT_SUPPORTED,
	GS_ERROR_LTS_ALLOCATE_BLOCK_FAILED,
	GS_ERROR_LTS_SWAP_IN_BLOCK_FAILED,
	GS_ERROR_LTS_SWAP_OUT_BLOCK_FAILED,
	GS_ERROR_LTS_INVALID_POSITION,
	GS_ERROR_LTS_BLOCK_VALIDATION_FAILED,
	GS_ERROR_LTS_TUPLELIST_LIMIT_EXCEEDED,
	GS_ERROR_LTS_LIMIT_EXCEEDED,

	GS_ERROR_NOSQL_INTERNAL = 420000,
	GS_ERROR_NOSQL_INVALID_OPERATION,
	GS_ERROR_NOSQL_FAILOVER_TIMEOUT,
	GS_ERROR_SQL_PARTITION_NOT_AVAILABLE
};

enum SQLTraceCode {
	GS_TRACE_SQL_INTERNAL_DEBUG = 200900,
	GS_TRACE_SQL_CONNECTION_ALREADY_CLOSED,
	GS_TRACE_SQL_STATEMENT_ALREADY_CLOSED,
	GS_TRACE_SQL_LONG_EVENT_WAITING,
	GS_TRACE_SQL_CONTINUABLE_ERROR,
	GS_TRACE_SQL_DDL_OPERATION,
	GS_TRACE_SQL_CANCEL,
	GS_TRACE_SQL_NOSQL_OPERATION_ALREADY_CANCEL,
	GS_TRACE_SQL_FAILOVER_WORKING,
	GS_TRACE_SQL_EXECUTION_INFO,
	GS_TRACE_SQL_DDL_TABLE_PARTITIONING_PRIMARY_KEY,
	GS_TRACE_SQL_RECOVER_CONTAINER,
	GS_TRACE_SQL_INVALID_NOSQL_RESPONSE,
	GS_TRACE_SQL_LONG_QUERY,
	GS_TRACE_SQL_LONG_UPDATING_CACHE_TABLE,
	GS_TRACE_SQL_HINT_INTERNAL = 240900,
	GS_TRACE_SQL_HINT_WARNING,
	GS_TRACE_SQL_HINT_INFO
};

class SQLState {
public:
	enum Code {
		STATE_SUCCESSFUL_COMPLETION = 0x00,
		STATE_WARNING = 0x01,
		STATE_NO_DATA = 0x02,
		STATE_CONNECTION_EXCEPTION = 0x08,
		STATE_FEATURE_NOT_SUPPORTED = 0x0A,
		STATE_DATA_EXCEPTION = 0x22,
		STATE_INTEGRITY_CONSTRAINT_VIOLATION = 0x23,
		STATE_INVALID_TRANSACTION_STATE = 0x25,
		STATE_INVALID_AUTHORIZATION_SPECIFICATION = 0x28,
		STATE_INVALID_SQL_DESCRIPTOR_NAME = 0x33,
		STATE_SQL_SYNTAX_ERROR = 0x37,
		STATE_INVALID_CATALOG_NAME = 0x3D,
		STATE_INVALID_SCHEMA_NAME = 0x3F,
		STATE_TRANSACTION_ROLLBACK = 0x40,
		STATE_ACCESS_RULE_VIOLATION = 0x42,
		STATE_INVALID_SYSTEM_STATE = 0x71,
		STATE_INSUFFICIENT_RESOURCES = 0x72,
		STATE_OUT_OF_MEMORY = 0x73,
		STATE_IO_ERROR = 0x74,
		STATE_SYSTEM_ERROR = 0x76,
		STATE_SQL_SYSTEM_ERROR = 0x77,
		STATE_UNKNOWN = 0xff
	};

	static util::Exception::NamedErrorCode sqlErrorToCode(
		int32_t sqliteErrorCode) throw();

	static util::Exception::NamedErrorCode sqlStateToCode(
		uint32_t sqlState) throw();

private:
	struct StateInfo {
		uint32_t code_;
		const char8_t* name_;
	};

	static util::Exception::NamedErrorCode stateInfoToCode(
		const StateInfo& info) throw();

	static const uint32_t EXCEPTION_CODE_STATE_BITS;

	static const StateInfo STATE_INFO_LIST[];
};

#define GS_THROW_SQL_USER_ERROR(sqlState, message) \
	UTIL_EXCEPTION_CREATE_DETAIL( \
			UserException, SQLState::sqlStateToCode(sqlState), NULL, message)

#define GS_SQL_CHECK_LOCAL_EXCEPTION(localException, extraMessage) \
	do { \
		if (localException.check() != NULL) { \
			try { \
				throw *localException.check(); \
			} \
			catch (std::exception &e) { \
				GS_RETHROW_USER_ERROR(e, GS_EXCEPTION_MESSAGE(e) << \
						extraMessage); \
			} \
		} \
	} \
	while (false)

struct SQLErrorUtils {
public:
	static void decodeException(
		util::StackAllocator& alloc, util::ArrayByteInStream& in,
		util::Exception& dest) throw();
};

#define GS_THROW_SQL_ERROR_BY_STREAM(alloc, in) \
	do { \
		try { \
			util::Exception e; \
			SQLErrorUtils::decodeException(alloc, in, e); \
			throw e; \
		} \
		catch (std::exception &e) { \
			GS_RETHROW_USER_ERROR(e, ""); \
		} \
	} \
	while (false)

enum SQLConfigTableParamId {
	CONFIG_TABLE_SQL_PARAM_START = CONFIG_TABLE_PARAM_END + 1,

	CONFIG_TABLE_SQL,

	CONFIG_TABLE_SQL_NOTIFICATION_ADDRESS,
	CONFIG_TABLE_SQL_NOTIFICATION_PORT,
	CONFIG_TABLE_SQL_NOTIFICATION_INTERVAL,

	CONFIG_TABLE_SQL_SERVICE_ADDRESS,
	CONFIG_TABLE_SQL_SERVICE_PORT,
	CONFIG_TABLE_SQL_CONNECTION_LIMIT,
	CONFIG_TABLE_SQL_CONCURRENCY,
	CONFIG_TABLE_SQL_TOTAL_MEMORY_LIMIT,
	CONFIG_TABLE_SQL_WORK_MEMORY_LIMIT,
	CONFIG_TABLE_SQL_STORE_MEMORY_LIMIT,
	CONFIG_TABLE_SQL_STORE_SWAP_FILE_SIZE_LIMIT,
	CONFIG_TABLE_SQL_STORE_SWAP_SYNC_SIZE,
	CONFIG_TABLE_SQL_STORE_SWAP_SYNC_INTERVAL,
	CONFIG_TABLE_SQL_STORE_SWAP_RELEASE_INTERVAL,
	CONFIG_TABLE_SQL_WORK_CACHE_MEMORY,
	CONFIG_TABLE_SQL_USE_KEEPALIVE,
	CONFIG_TABLE_SQL_KEEPALIVE_IDLE,
	CONFIG_TABLE_SQL_KEEPALIVE_INTERVAL,
	CONFIG_TABLE_SQL_KEEPALIVE_COUNT,
	CONFIG_TABLE_SQL_FETCH_SIZE_LIMIT,
	CONFIG_TABLE_SQL_TRANSACTION_MESSAGE_SIZE_LIMIT,
	CONFIG_TABLE_SQL_STORE_SWAP_FILE_PATH,
	CONFIG_TABLE_SQL_SUB_CONTAINER_ASSIGN_TYPE,
	CONFIG_TABLE_SQL_SCAN_RESULTSET_TIMEOUT_INTERVAL,
	CONFIG_TABLE_SQL_TRACE_LIMIT_EXECUTION_TIME,
	CONFIG_TABLE_SQL_TRACE_LIMIT_QUERY_SIZE,
	CONFIG_TABLE_SQL_SEND_PENDING_INTERVAL,
	CONFIG_TABLE_SQL_SEND_PENDING_TASK_LIMIT,
	CONFIG_TABLE_SQL_SEND_PENDING_JOB_LIMIT,
	CONFIG_TABLE_SQL_SEND_PENDING_TASK_CONCURRENCY,
	CONFIG_TABLE_SQL_JOB_TOTAL_MEMORY_LIMIT,

	CONFIG_TABLE_SQL_NOSQL_FAILOVER_TIMEOUT,

	CONFIG_TABLE_SQL_MULTI_INDEX_SCAN,
	CONFIG_TABLE_SQL_PARTITIONING_ROWKEY_CONSTRAINT,
	CONFIG_TABLE_SQL_PLAN_VERSION,
	CONFIG_TABLE_SQL_COST_BASED_JOIN,

	CONFIG_TABLE_SQL_LOCAL_SERVICE_ADDRESS,
	CONFIG_TABLE_SQL_NOTIFICATION_INTERFACE_ADDRESS,

	CONFIG_TABLE_SQL_TABLE_CACHE_EXPIRED_TIME,
	CONFIG_TABLE_SQL_TABLE_CACHE_SIZE,

	CONFIG_TABLE_SQL_ENABLE_PROFILER,
	CONFIG_TABLE_SQL_ENABLE_JOB_MEMORY_CHECK_INTERVAL_COUNT,
	CONFIG_TABLE_SQL_TABLE_CACHE_LOAD_DUMP_LIMIT_TIME,

	CONFIG_TABLE_SQL_PUBLIC_SERVICE_ADDRESS,
	CONFIG_TABLE_SQL_ENABLE_SCAN_META_TABLE_ADMINISTRATOR_PRIVILEGES,
	CONFIG_TABLE_SQL_ADD_BATCH_MAX_COUNT,
	CONFIG_TABLE_SQL_TABLE_PARTITIONING_MAX_ASSIGN_NUM,
	CONFIG_TABLE_SQL_TABLE_PARTITIONING_MAX_ASSIGN_ENTRY_NUM,

	CONFIG_TABLE_SQL_FAIL_ON_TOTAL_MEMORY_LIMIT,
	CONFIG_TABLE_SQL_WORK_MEMORY_RATE,
	CONFIG_TABLE_SQL_RESOURCE_CONTROL_LEVEL,
	CONFIG_TABLE_SQL_EVENT_BUFFER_CACHE_SIZE,

	CONFIG_TABLE_TRACE_SQL_TRACER_ID_START,

	CONFIG_TABLE_TRACE_SQL_SERVICE,
	CONFIG_TABLE_TRACE_SQL_TEMP_STORE,
	CONFIG_TABLE_TRACE_TUPLE_LIST,
	CONFIG_TABLE_TRACE_DISTRIBUTED_FRAMEWORK,
	CONFIG_TABLE_TRACE_DISTRIBUTED_FRAMEWORK_DETAIL,
	CONFIG_TABLE_TRACE_SQL_HINT,

	CONFIG_TABLE_TRACE_SQL_INTERNAL,

	CONFIG_TABLE_TRACE_SQL_TRACER_ID_END,

	CONFIG_TABLE_SQL_PARAM_END
};

enum SQLStatTableParamId {
	STAT_TABLE_SQL_PARAM_START = STAT_TABLE_PARAM_END + 1,

	STAT_TABLE_PERF_MEM_WORK_SQL_MESSAGE_TOTAL,
	STAT_TABLE_PERF_MEM_WORK_SQL_MESSAGE_CACHED,
	STAT_TABLE_PERF_MEM_WORK_SQL_WORK_TOTAL,
	STAT_TABLE_PERF_MEM_WORK_SQL_WORK_CACHED,
	STAT_TABLE_PERF_MEM_WORK_SQL_STORE_TOTAL,
	STAT_TABLE_PERF_MEM_WORK_SQL_STORE_CACHED,
	STAT_TABLE_PERF_MEM_WORK_SQL_JOB_TOTAL,
	STAT_TABLE_PERF_MEM_WORK_SQL_JOB_CACHED,
	STAT_TABLE_PERF_SQL_STORE_SWAP_FILE_SIZE_TOTAL,
	STAT_TABLE_PERF_SQL_TOTAL_READ_OPERATION,
	STAT_TABLE_PERF_SQL_TOTAL_WRITE_OPERATION,
	STAT_TABLE_PERF_SQL_NUM_CONNECTION,
	STAT_TABLE_PERF_SQL_SEND_PENDING_COUNT,
	STAT_TABLE_PERF_SQL_JOB_COUNT,
	STAT_TABLE_PERF_SQL_SQL_COUNT,
	STAT_TABLE_PERF_SQL_SQL_RETRY_COUNT, 
	STAT_TABLE_PERF_SQL_QUEUE_MAX,
	STAT_TABLE_PERF_SQL_TABLE_CACHE_RATIO,
	STAT_TABLE_PERF_SQL_TABLE_CACHE_SEARCH_COUNT,
	STAT_TABLE_PERF_SQL_TABLE_CACHE_SKIP_COUNT,
	STAT_TABLE_PERF_SQL_TABLE_CACHE_LOAD_TIME,

	STAT_TABLE_PERF_SQL_STORE_SWAP_READ,
	STAT_TABLE_PERF_SQL_STORE_SWAP_READ_SIZE,
	STAT_TABLE_PERF_SQL_STORE_SWAP_READ_TIME,
	STAT_TABLE_PERF_SQL_STORE_SWAP_WRITE,
	STAT_TABLE_PERF_SQL_STORE_SWAP_WRITE_SIZE,
	STAT_TABLE_PERF_SQL_STORE_SWAP_WRITE_TIME,

	STAT_TABLE_PERF_SQL_TOTAL_INTERNAL_CONNECTION_COUNT,
	STAT_TABLE_PERF_SQL_TOTAL_EXTERNAL_CONNECTION_COUNT,

	STAT_TABLE_SQL_PARAM_END
};

enum SQLAllocatorGroupId {
	ALLOCATOR_GROUP_SQL_ID_START = ALLOCATOR_GROUP_ID_END,

	ALLOCATOR_GROUP_SQL_MESSAGE,
	ALLOCATOR_GROUP_SQL_WORK,
	ALLOCATOR_GROUP_SQL_LTS,
	ALLOCATOR_GROUP_SQL_JOB,

	ALLOCATOR_GROUP_SQL_ID_END
};

struct NoSQLCommonUtils {
	static bool isAccessibleContainer(uint8_t attribute, bool& isWritable);
	static bool isWritableContainer(uint8_t attribute, uint8_t type);

	static void splitContainerName(
		const std::pair<const char8_t*, const char8_t*>& src,
		std::pair<const char8_t*, const char8_t*>* base,
		std::pair<const char8_t*, const char8_t*>* sub,
		std::pair<const char8_t*, const char8_t*>* affinity,
		std::pair<const char8_t*, const char8_t*>* type);

	/*!
		@brief コンテナ名よりサブコンテナ名を生成する
	*/
	static void makeSubContainerName(
		const char8_t* name, PartitionId partitionId, int32_t subContainerId,
		int32_t tablePartitioningCount, util::String& subName);

	static int32_t mapTypeToSQLIndexFlags(
		int8_t mapType, uint8_t nosqlColumnType);

	static void getDatabaseVersion(int32_t& major, int32_t& minor);
};

struct SQLPragma {
	static const char8_t VALUE_TRUE[];
	static const char8_t VALUE_FALSE[];

	enum PragmaType {
		PRAGMA_NONE = 0,
		PRAGMA_INTERNAL_COMPILER_EXECUTE_AS_META_DATA_QUERY,
		PRAGMA_INTERNAL_COMPILER_SCAN_INDEX,
		PRAGMA_INTERNAL_COMPILER_META_TABLE_VISIBLE,
		PRAGMA_INTERNAL_COMPILER_INTERNAL_META_TABLE_VISIBLE,
		PRAGMA_INTERNAL_COMPILER_DRIVER_META_TABLE_VISIBLE,

		PRAGMA_INTERNAL_COMPILER_SUBSTR_COMPATIBLE,
		PRAGMA_INTERNAL_COMPILER_VARIANCE_COMPATIBLE,
		PRAGMA_INTERNAL_COMPILER_EXPERIMENTAL_FUNCTIONS,

		PRAGMA_INTERNAL_PARSER_TOKEN_COUNT_LIMIT,
		PRAGMA_INTERNAL_PARSER_EXPR_TREE_DEPTH_LIMIT,

		PRAGMA_PLAN_DISTRIBUTED_STRATEGY,

		PRAGMA_EXPERIMENTAL_SHOW_SYSTEM,
		PRAGMA_EXPERIMENTAL_SCAN_MULTI_INDEX,

		PRAGMA_RESULTSET_TIMEOUT_INTERVAL,
		PRAGMA_TYPE_MAX
	};

	static const char* getPragmaTypeName(PragmaType type) {
		switch (type) {
		case PRAGMA_NONE: return "none";
		case PRAGMA_INTERNAL_COMPILER_EXECUTE_AS_META_DATA_QUERY: return "internal.compiler.execute_as_meta_data_query";
		case PRAGMA_INTERNAL_COMPILER_SCAN_INDEX: return "internal.compiler.scan_index";
		case PRAGMA_INTERNAL_COMPILER_META_TABLE_VISIBLE:return "internal.compiler.meta_table_visible";
		case PRAGMA_INTERNAL_COMPILER_INTERNAL_META_TABLE_VISIBLE:return "internal.compiler.internal_meta_table_visible";
		case PRAGMA_INTERNAL_COMPILER_DRIVER_META_TABLE_VISIBLE:return "internal.compiler.driver_meta_table_visible";
		case PRAGMA_INTERNAL_COMPILER_SUBSTR_COMPATIBLE:return "internal.compiler.substr_compatible";
		case PRAGMA_INTERNAL_COMPILER_VARIANCE_COMPATIBLE:return "internal.compiler.variance_compatible";
		case PRAGMA_INTERNAL_COMPILER_EXPERIMENTAL_FUNCTIONS:return "internal.compiler.experimental_functions";
		case PRAGMA_INTERNAL_PARSER_TOKEN_COUNT_LIMIT: return "internal.parser.token_count_limit";
		case PRAGMA_INTERNAL_PARSER_EXPR_TREE_DEPTH_LIMIT: return "internal.parser.expr_tree_depth_limit";
		case PRAGMA_PLAN_DISTRIBUTED_STRATEGY: return "plan.distributed.strategy";
		case PRAGMA_EXPERIMENTAL_SHOW_SYSTEM: return "experimental.show.system";
		case PRAGMA_EXPERIMENTAL_SCAN_MULTI_INDEX: return "experimental.scan.multi_index";
		case PRAGMA_RESULTSET_TIMEOUT_INTERVAL: return "internal.resultset.timeout";
		default: return "undefined";
		}
	}

	static PragmaType getPragmaType(const char* pragmaName) {
		if (!strcmp(pragmaName, "internal.compiler.execute_as_meta_data_query")) {
			return PRAGMA_INTERNAL_COMPILER_EXECUTE_AS_META_DATA_QUERY;
		}
		else if (!strcmp(pragmaName, "internal.compiler.scan_index")) {
			return PRAGMA_INTERNAL_COMPILER_SCAN_INDEX;
		}
		else if (!strcmp(pragmaName, "internal.compiler.meta_table_visible")) {
			return PRAGMA_INTERNAL_COMPILER_META_TABLE_VISIBLE;
		}
		else if (!strcmp(pragmaName, "internal.compiler.internal_meta_table_visible")) {
			return PRAGMA_INTERNAL_COMPILER_INTERNAL_META_TABLE_VISIBLE;
		}
		else if (!strcmp(pragmaName, "internal.compiler.driver_meta_table_visible")) {
			return PRAGMA_INTERNAL_COMPILER_DRIVER_META_TABLE_VISIBLE;
		}
		else if (!strcmp(pragmaName, "internal.compiler.substr_compatible")) {
			return PRAGMA_INTERNAL_COMPILER_SUBSTR_COMPATIBLE;
		}
		else if (!strcmp(pragmaName, "internal.compiler.variance_compatible")) {
			return PRAGMA_INTERNAL_COMPILER_VARIANCE_COMPATIBLE;
		}
		else if (!strcmp(pragmaName, "internal.compiler.experimental_functions")) {
			return PRAGMA_INTERNAL_COMPILER_EXPERIMENTAL_FUNCTIONS;
		}
		else if (!strcmp(pragmaName, "internal.parser.token_count_limit")) {
			return PRAGMA_INTERNAL_PARSER_TOKEN_COUNT_LIMIT;
		}
		else if (!strcmp(pragmaName, "internal.parser.expr_tree_depth_limit")) {
			return PRAGMA_INTERNAL_PARSER_EXPR_TREE_DEPTH_LIMIT;
		}
		else if (!strcmp(pragmaName, "plan.distributed.strategy")) {
			return PRAGMA_PLAN_DISTRIBUTED_STRATEGY;
		}
		else if (!strcmp(pragmaName, "experimental.show.system")) {
			return PRAGMA_EXPERIMENTAL_SHOW_SYSTEM;
		}
		else if (!strcmp(pragmaName, "experimental.scan.multi_index")) {
			return PRAGMA_EXPERIMENTAL_SCAN_MULTI_INDEX;
		}
		else if (!strcmp(pragmaName, "internal.resultset.timeout")) {
			return PRAGMA_RESULTSET_TIMEOUT_INTERVAL;
		}
		else {
			return PRAGMA_NONE;
		}
	}
};

struct NameWithCaseSensitivity {
	NameWithCaseSensitivity(const char8_t* name, bool isCaseSensitive = false) :
		name_(name), isCaseSensitive_(isCaseSensitive) {}

	const char8_t* name_;
	bool isCaseSensitive_;
};

struct TaskProfiler {

	TaskProfiler();

	void copy(util::StackAllocator& alloc, const TaskProfiler& target);

	UTIL_OBJECT_CODER_MEMBERS(
		leadTime_, actualTime_, executionCount_,
		worker_, rows_, address_, customData_);

	int64_t leadTime_;
	int64_t actualTime_;
	int64_t executionCount_;
	int32_t worker_;
	util::Vector<int64_t>* rows_;
	util::String* address_;
	util::XArray<uint8_t>* customData_;
};

int64_t convertToTime(util::String& timeStr);

template<typename T>
std::string dumpObject(util::StackAllocator& alloc, T& object);


#endif
