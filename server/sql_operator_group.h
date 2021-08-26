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

#ifndef SQL_OPERATOR_GROUP_H_
#define SQL_OPERATOR_GROUP_H_

#include "sql_operator.h"

#include "sql_operator_utils.h"
#include "sql_utils_algorithm.h"

struct SQLGroupOps {
	typedef SQLOps::TupleListReader TupleListReader;
	typedef SQLOps::TupleListWriter TupleListWriter;

	typedef SQLOps::SummaryTuple SummaryTuple;
	typedef SQLOps::ReadableTuple ReadableTuple;
	typedef SQLOps::WritableTuple WritableTuple;
	typedef SQLOps::TupleColumn TupleColumn;

	typedef SQLOps::ColumnTypeList ColumnTypeList;
	typedef SQLOps::TupleColumnList TupleColumnList;

	typedef SQLOps::DigestTupleListReader DigestTupleListReader;
	typedef SQLOps::DigestReadableTuple DigestReadableTuple;

	typedef SQLOps::OpCode OpCode;
	typedef SQLOps::OpNode OpNode;
	typedef SQLOps::OpPlan OpPlan;

	typedef SQLOps::OpContext OpContext;
	typedef SQLOps::OpCodeBuilder OpCodeBuilder;

	typedef SQLOps::ProjectionCode ProjectionCode;

	typedef SQLOps::Projection Projection;
	typedef SQLOps::ProjectionFactoryContext ProjectionFactoryContext;

	typedef SQLOpUtils::ProjectionPair ProjectionPair;

	typedef SQLValues::TupleDigester::WithAccessor<
			SQLValues::ValueAccessor::ByReader, 1, true> UniqTupleDigester;
	typedef SQLValues::ReadableTupleRef::WithDigester<
			UniqTupleDigester> ReadableTupleRef;
	typedef SQLValues::TupleComparator::WithAccessor<
			std::less<SQLValues::ValueComparator::PredArgType>,
			true, false, false, false,
			SQLValues::ValueAccessor::ByReader> TupleGreater;
	typedef SQLAlgorithmUtils::HeapQueue<
			ReadableTupleRef, TupleGreater> TupleHeapQueue;

	class Registrar;

	class Group;
	class GroupDistinct;
	class GroupDistinctMerge;
	class GroupBucketHash;

	class Union;
	class UnionAll;

	struct UnionMergeContext;
	template<typename Op> class UnionMergeBase;
	class UnionDistinct;
	class UnionIntersect;
	class UnionExcept;
	class UnionCompensate;

	class UnionHashContext;
	template<typename Op> class UnionHashBase;
	class UnionDistinctHash;
	class UnionIntersectHash;
	class UnionExceptHash;
};

class SQLGroupOps::Registrar : public SQLOps::OpProjectionRegistrar {
public:
	virtual void operator()() const;

private:
	static const SQLOps::OpProjectionRegistrar REGISTRAR_INSTANCE;
};


class SQLGroupOps::Group : public SQLOps::Operator {
public:
	virtual void compile(OpContext &cxt) const;

private:
	static void setUpGroupKeys(
			OpCodeBuilder &builder, OpCode &code,
			const SQLValues::CompColumnList &keyList, bool distinct);

	static SQLValues::CompColumnList* createDistinctGroupKeys(
			OpContext &cxt, bool distinct, SQLType::AggregationPhase aggrPhase,
			const Projection *srcProj,
			const SQLValues::CompColumnList &srcList);

	static void setUpGroupProjections(
			OpCodeBuilder &builder, OpCode &code, const Projection &src,
			const SQLValues::CompColumnList &keyList,
			const SQLValues::CompColumnList &midKeyList);

	static void setUpDistinctGroupProjections(
			OpCodeBuilder &builder, OpCode &code,
			const SQLValues::CompColumnList &midKeyList,
			Projection **distinctProj);
};

class SQLGroupOps::GroupDistinct : public SQLOps::Operator {
public:
	virtual void compile(OpContext &cxt) const;
};

class SQLGroupOps::GroupDistinctMerge : public SQLOps::Operator {
public:
	virtual void execute(OpContext &cxt) const;

private:
	struct MergeContext {
		MergeContext();

		bool merged_;
		bool nextReaderStarted_;
	};

	const SQLValues::CompColumn& getIdKeyColumn() const;
	const Projection& getProjections(
			SQLOpUtils::ProjectionRefPair &emptyProjs) const;

	MergeContext& prepareMergeContext(OpContext &cxt) const;
};

class SQLGroupOps::GroupBucketHash : public SQLOps::Operator {
public:
	virtual void execute(OpContext &cxt) const;

private:
	typedef SQLValues::TupleDigester::WithAccessor<
			SQLValues::ValueAccessor::ByReader, 1, false> BucketTupleDigester;

	typedef util::Vector<SQLOpUtils::ExpressionListWriter*> WriterList;

	struct BucketContext {
		BucketContext(
				util::StackAllocator &alloc, SQLValues::VarContext &varCxt,
				const SQLValues::CompColumnList &keyList);

		SQLOpUtils::ExpressionListWriter::ByGeneral getOutput(uint64_t index);

		static SQLValues::TupleDigester createBaseDigester(
				SQLValues::VarContext &varCxt,
				const SQLValues::CompColumnList &keyList);

		WriterList writerList_;
		BucketTupleDigester digester_;
	};

	BucketContext& prepareContext(OpContext &cxt) const;
};

class SQLGroupOps::Union : public SQLOps::Operator {
public:
	virtual void compile(OpContext &cxt) const;

private:
	bool tryCompileHashPlan(OpContext &cxt) const;
	bool tryCompileEmptyPlan(OpContext &cxt) const;

	static bool checkHashPlanAcceptable(
			OpContext &cxt, OpCodeBuilder &builder, const OpCode &code,
			SQLOpTypes::Type *hashOpType,
			const SQLValues::CompColumnList **keyList);

	static SQLOpTypes::Type toOperatorType(SQLType::UnionType unionType);
	static SQLOpTypes::Type findHashOperatorType(
			SQLType::UnionType unionType, uint32_t inCount);

	static const SQLValues::CompColumnList& resolveKeyColumnList(
			OpContext &cxt, OpCodeBuilder &builder,
			const SQLValues::CompColumnList *src, bool withAttributes);

	static bool checkKeySimple(const SQLValues::CompColumnList &keyList);
	static bool checkInputSimple(
			const TupleColumnList &columnList,
			const SQLValues::CompColumnList &keyList);
	static bool checkOutputSimple(
		const Projection *pipeProj, const Projection *finishProj);

	static uint64_t estimateHashMapSize(
			OpContext &cxt, const SQLValues::CompColumnList &keyList,
			int64_t tupleCount);
};

class SQLGroupOps::UnionAll : public SQLOps::Operator {
public:
	virtual void compile(OpContext &cxt) const;
	virtual void execute(OpContext &cxt) const;

private:
	typedef std::pair<
			SQLOpUtils::ExpressionListWriter*, const Projection*> WriterEntry;

	typedef util::Vector<WriterEntry*> WriterList;
	typedef util::Map<ColumnTypeList, WriterEntry*> WriterMap;

	struct UnionAllContext {
		UnionAllContext(util::StackAllocator &alloc, uint32_t inCount);
		WriterList writerList_;
		WriterMap writerMap_;
	};

	WriterEntry& prepareWriter(OpContext &cxt, uint32_t index) const;

	static void getInputColumnTypeList(
			OpContext &cxt, uint32_t index, ColumnTypeList &typeList);
};

struct SQLGroupOps::UnionMergeContext {
	explicit UnionMergeContext(int64_t initialState);

	TupleHeapQueue createHeapQueue(
			OpContext &cxt,
			const SQLValues::CompColumnList &keyColumnList,
			util::LocalUniquePtr<TupleHeapQueue::Element> *topElem);

	int64_t state_;
	int64_t initialState_;
	bool topElemChecked_;
};

template<typename Op>
class SQLGroupOps::UnionMergeBase : public SQLOps::Operator {
public:
	typedef util::TrueType InputUnique;

	virtual void compile(OpContext &cxt) const;
	virtual void execute(OpContext &cxt) const;

protected:
	static bool onTuple(int64_t &state, size_t ordinal);
	static bool onFinish(int64_t &state, int64_t initialState);
	static bool onSingle(size_t ordinal, int64_t initialState);
	static int64_t toInitial(OpContext &cxt, const OpCode &code);

private:
	struct MergeAction;
};

template<typename Op>
struct SQLGroupOps::UnionMergeBase<Op>::MergeAction {
	struct Options {
		typedef util::FalseType FixedDigest;
	};
	typedef Options OptionsType;

	template<typename T>
	struct TypeAt {
		typedef MergeAction TypedOp;
	};

	MergeAction(
			OpContext &cxt, UnionMergeContext &unionCxt,
			const SQLOps::Projection *projection,
			TupleHeapQueue::Element **topElemPtr);

	bool operator()(
			const TupleHeapQueue::Element &elem, const util::FalseType&);
	void operator()(
			const TupleHeapQueue::Element &elem, const util::TrueType&);

	bool operator()(
			const TupleHeapQueue::Element &elem, const util::TrueType&,
			const util::TrueType&);

	template<typename Pred>
	bool operator()(
			const TupleHeapQueue::Element &elem, const util::TrueType&,
			const Pred &pred);

	OpContext &cxt_;
	UnionMergeContext &unionCxt_;
	const SQLOps::Projection *projection_;
	TupleHeapQueue::Element **topElemPtr_;
};

class SQLGroupOps::UnionDistinct :
		public SQLGroupOps::UnionMergeBase<UnionDistinct> {
public:
};

class SQLGroupOps::UnionIntersect :
		public SQLGroupOps::UnionMergeBase<UnionIntersect> {
public:
	static bool onTuple(int64_t &state, size_t ordinal);
	static bool onFinish(int64_t &state, int64_t initialState);
	static bool onSingle(size_t ordinal, int64_t initialState);
	static int64_t toInitial(OpContext &cxt, const OpCode &code);
};

class SQLGroupOps::UnionExcept :
		public SQLGroupOps::UnionMergeBase<UnionExcept> {
public:
	static bool onTuple(int64_t &state, size_t ordinal);
	static bool onFinish(int64_t &state, int64_t initialState);
	static bool onSingle(size_t ordinal, int64_t initialState);
};

class SQLGroupOps::UnionCompensate :
		public SQLGroupOps::UnionMergeBase<UnionCompensate> {
public:
	typedef util::FalseType InputUnique;
	static bool onTuple(int64_t &state, size_t ordinal);
	static bool onFinish(int64_t &state, int64_t initialState);
};

class SQLGroupOps::UnionHashContext {
public:
	struct InputEntry;

	static UnionHashContext& resolve(
			OpContext &cxt, const SQLValues::CompColumnList &keyList,
			const Projection &baseProj, const SQLOps::OpConfig *config);

	UnionHashContext(
			OpContext &cxt, const SQLValues::CompColumnList &keyList,
			const Projection &baseProj, const SQLOps::OpConfig *config);

	template<typename Op>
	const Projection* accept(
			TupleListReader &reader, const uint32_t index, InputEntry &entry);

	void checkMemoryLimit();
	InputEntry& prepareInput(OpContext &cxt, const uint32_t index);

	bool isTopInputPending();
	void setTopInputPending(bool pending);

	bool isFollowingInputCompleted(OpContext &cxt);

private:
	typedef SQLValues::TupleListReaderSource ReaderSourceType;
	typedef SQLValues::SummaryTupleSet SummaryTupleSet;

	typedef SQLValues::TupleDigester::WithAccessor<
			SQLValues::ValueAccessor::ByReader, 1, false> TupleDigester;
	typedef SQLValues::TupleComparator::WithAccessor<
			std::equal_to<SQLValues::ValueComparator::PredArgType>,
			false, true, false, false,
			SQLValues::ValueAccessor::ByReader,
			SQLValues::ValueAccessor::BySummaryTuple> TupleEq;

	typedef SQLValues::DigestHasher MapHasher;
	typedef std::equal_to<int64_t> MapPred;
	typedef std::pair<SummaryTuple, int64_t> MapEntry;
	typedef SQLAlgorithmUtils::HashMultiMap<
			int64_t, MapEntry, MapHasher, MapPred> Map;

	typedef util::Vector<InputEntry*> InputEntryList;

	static uint64_t resolveMemoryLimit(const SQLOps::OpConfig *config);
	static const Projection& createProjection(
			OpContext &cxt, const Projection &baseProj,
			const SQLValues::CompColumnList &keyList, bool digestOrdering);
	static void initializeTupleSet(
			OpContext &cxt, const SQLValues::CompColumnList &keyList,
			util::LocalUniquePtr<SummaryTupleSet> &tupleSet,
			bool digestOrdering);

	static void setUpInputEntryList(
			OpContext &cxt, const SQLValues::CompColumnList &keyList,
			const Projection &baseProj, InputEntryList &entryList,
			bool digestOrdering);
	static bool checkDigestOrdering(
			OpContext &cxt, const SQLValues::CompColumnList &keyList);

	util::StackAllocator &alloc_;

	uint64_t memoryLimit_;
	bool memoryLimitReached_;

	bool topInputPending_;
	bool followingInputCompleted_;

	util::LocalUniquePtr<SummaryTupleSet> tupleSet_;

	Map map_;
	InputEntryList inputEntryList_;

	const Projection *proj_;
	SummaryTuple *projTupleRef_;
};

struct SQLGroupOps::UnionHashContext::InputEntry {
public:
	InputEntry(
			OpContext &cxt, const SQLValues::CompColumnList &digesterKeyList,
			bool digestOrdering, const SQLValues::CompColumnList &eqKeyList,
			const Projection &subProj);

	TupleDigester digester_;
	TupleEq tupleEq_;
	const Projection &subProj_;
};

template<typename Op>
class SQLGroupOps::UnionHashBase : public SQLOps::Operator {
public:
	virtual void compile(OpContext &cxt) const;
	virtual void execute(OpContext &cxt) const;

protected:
	typedef util::FalseType TopInputCascading;
	static bool onTuple(int64_t &state, size_t ordinal);
};

class SQLGroupOps::UnionDistinctHash :
		public SQLGroupOps::UnionHashBase<UnionDistinctHash> {
public:
	static bool onTuple(int64_t &state, size_t ordinal);
};

class SQLGroupOps::UnionIntersectHash :
		public SQLGroupOps::UnionHashBase<UnionIntersectHash> {
public:
	static bool onTuple(int64_t &state, size_t ordinal);
};

class SQLGroupOps::UnionExceptHash :
		public SQLGroupOps::UnionHashBase<UnionExceptHash> {
public:
	typedef util::TrueType TopInputCascading;
	static bool onTuple(int64_t &state, size_t ordinal);
};

#endif
