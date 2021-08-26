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

#include "sql_operator_group.h"



const SQLOps::OpProjectionRegistrar
SQLGroupOps::Registrar::REGISTRAR_INSTANCE((Registrar()));

void SQLGroupOps::Registrar::operator()() const {
	add<SQLOpTypes::OP_GROUP, Group>();
	add<SQLOpTypes::OP_GROUP_DISTINCT, GroupDistinct>();
	add<SQLOpTypes::OP_GROUP_DISTINCT_MERGE, GroupDistinctMerge>();
	add<SQLOpTypes::OP_GROUP_BUCKET_HASH, GroupBucketHash>();

	add<SQLOpTypes::OP_UNION, Union>();
	add<SQLOpTypes::OP_UNION_ALL, UnionAll>();

	add<SQLOpTypes::OP_UNION_DISTINCT_MERGE, UnionDistinct>();
	add<SQLOpTypes::OP_UNION_INTERSECT_MERGE, UnionIntersect>();
	add<SQLOpTypes::OP_UNION_EXCEPT_MERGE, UnionExcept>();
	add<SQLOpTypes::OP_UNION_COMPENSATE_MERGE, UnionCompensate>();

	add<SQLOpTypes::OP_UNION_DISTINCT_HASH, UnionDistinctHash>();
	add<SQLOpTypes::OP_UNION_INTERSECT_HASH, UnionIntersectHash>();
	add<SQLOpTypes::OP_UNION_EXCEPT_HASH, UnionExceptHash>();
}


void SQLGroupOps::Group::compile(OpContext &cxt) const {
	OpPlan &plan = cxt.getPlan();

	OpNode &node = plan.createNode(SQLOpTypes::OP_SORT);

	node.addInput(plan.getParentInput(0));

	OpCode code = getCode();
	OpCodeBuilder builder(OpCodeBuilder::ofContext(cxt));

	const Projection *srcProj = code.getPipeProjection();
	const bool distinct = builder.findDistinctAggregation(*srcProj);

	{
		const SQLValues::CompColumnList &keyList = code.getKeyColumnList();
		setUpGroupKeys(builder, code, keyList, distinct);
	}

	Projection *distinctProj = NULL;
	{
		assert(srcProj != NULL);
		assert(code.getFinishProjection() == NULL);
		if (distinct) {
			setUpDistinctGroupProjections(
					builder, code, code.getMiddleKeyColumnList(),
					&distinctProj);
		}
		else {
			setUpGroupProjections(
					builder, code, *srcProj,
					code.getKeyColumnList(), code.getMiddleKeyColumnList());
		}
	}
	node.setCode(code);

	const SQLType::AggregationPhase aggrPhase =
			getCode().getAggregationPhase();
	SQLValues::CompColumnList *distinctKeyList = createDistinctGroupKeys(
			cxt, distinct, aggrPhase,
			code.getPipeProjection(), code.getKeyColumnList());

	OpCodeBuilder::setUpOutputNodes(
			plan, NULL, node, distinctProj, aggrPhase, distinctKeyList);
}

void SQLGroupOps::Group::setUpGroupKeys(
		OpCodeBuilder &builder, OpCode &code,
		const SQLValues::CompColumnList &keyList, bool distinct) {

	SQLExprs::ExprRewriter &rewriter = builder.getExprRewriter();
	SQLExprs::ExprFactoryContext &factoryCxt = builder.getExprFactoryContext();

	util::Set<uint32_t> keyPosSet(factoryCxt.getAllocator());

	SQLValues::CompColumnList &midKeyList = builder.createCompColumnList();
	midKeyList = rewriter.rewriteCompColumnList(factoryCxt, keyList, false);
	SQLExprs::ExprRewriter::normalizeCompColumnList(midKeyList, keyPosSet);

	const bool withDigest =
			!SQLValues::TupleDigester::isOrderingAvailable(midKeyList, false);

	code.setMiddleKeyColumnList(&midKeyList);

	SQLExprs::ExprRewriter::Scope scope(rewriter);
	rewriter.activateColumnMapping(factoryCxt);

	const uint32_t input = 0;

	if (!distinct) {
		rewriter.clearColumnUsage();
		rewriter.addKeyColumnUsage(input, midKeyList, true);
	}

	if (withDigest) {
		rewriter.setIdOfInput(input, true);
		rewriter.setIdProjected(true);
	}

	rewriter.setInputProjected(true);

	const bool keyOnly = !distinct;
	code.setKeyColumnList(&rewriter.remapCompColumnList(
			factoryCxt, midKeyList, input, true, keyOnly, &keyPosSet));
}

SQLValues::CompColumnList* SQLGroupOps::Group::createDistinctGroupKeys(
		OpContext &cxt, bool distinct, SQLType::AggregationPhase aggrPhase,
		const Projection *srcProj, const SQLValues::CompColumnList &srcList) {
	if (!distinct || aggrPhase != SQLType::AGG_PHASE_ADVANCE_PIPE) {
		return NULL;
	}

	const bool forPipe = true;
	const Projection *pipeProj = SQLOps::OpCodeBuilder::findAggregationProjection(
			*srcProj, &forPipe);

	assert(srcProj != NULL);
	const uint32_t outIndex = 0;
	const Projection *outProj =
			SQLOps::OpCodeBuilder::findOutputProjection(*srcProj, &outIndex);

	if (pipeProj == NULL || outProj == NULL) {
		assert(false);
		GS_THROW_USER_ERROR(GS_ERROR_SQL_PROC_INTERNAL, "");
	}

	util::StackAllocator &alloc = cxt.getAllocator();

	util::Set<uint32_t> aggrSet(alloc);
	{
		const util::Set<uint32_t> &keySet =
				SQLExprs::ExprRewriter::compColumnListToSet(alloc, srcList);
		for (SQLExprs::Expression::Iterator it(*pipeProj);
				it.exists(); it.next()) {
			const SQLExprs::Expression &expr = it.get();
			if (expr.getCode().getType() != SQLType::AGG_FIRST) {
				continue;
			}

			const SQLExprs::Expression *subExpr = expr.findChild();
			if (subExpr == NULL ||
					subExpr->getCode().getType() != SQLType::EXPR_COLUMN ||
					keySet.find(subExpr->getCode().getColumnPos()) ==
							keySet.end()) {
				continue;
			}

			aggrSet.insert(expr.getCode().getAggregationIndex());
		}
	}

	OpCodeBuilder builder(OpCodeBuilder::ofContext(cxt));
	SQLValues::CompColumnList &destList = builder.createCompColumnList();

	uint32_t destPos = 0;
	for (SQLExprs::Expression::Iterator it(*outProj); it.exists(); it.next()) {
		const SQLExprs::Expression &expr = it.get();

		if (expr.getCode().getType() == SQLType::AGG_FIRST &&
				aggrSet.find(expr.getCode().getAggregationIndex()) !=
						aggrSet.end()) {
			SQLValues::CompColumn column;
			column.setColumnPos(destPos, true);
			column.setOrdering(false);
			destList.push_back(column);
		}

		destPos++;
	}

	return &destList;
}

void SQLGroupOps::Group::setUpGroupProjections(
		OpCodeBuilder &builder, OpCode &code, const Projection &src,
		const SQLValues::CompColumnList &keyList,
		const SQLValues::CompColumnList &midKeyList) {
	SQLExprs::ExprFactoryContext &factoryCxt = builder.getExprFactoryContext();
	SQLExprs::ExprFactoryContext::Scope scope(factoryCxt);

	const SQLType::AggregationPhase srcPhase = code.getAggregationPhase();
	factoryCxt.setAggregationPhase(true, srcPhase);

	factoryCxt.setArrangedKeyList(&midKeyList, false);

	code.setMiddleProjection(&builder.createPipeFinishProjection(
			builder.createMultiStageProjection(
					builder.createMultiStageAggregation(src, -1, false, NULL),
					builder.createMultiStageAggregation(src, 0, false, NULL)),
			builder.createMultiStageAggregation(src, 0, true, &midKeyList)));

	Projection &mid = builder.createPipeFinishProjection(
			builder.createMultiStageAggregation(src, 1, false, NULL),
			builder.createMultiStageAggregation(src, 1, true, &keyList));

	Projection &pipe = builder.createPipeFinishProjection(
			builder.createMultiStageAggregation(src, 2, false, NULL),
			builder.createLimitedProjection(
					builder.createMultiStageAggregation(src, 2, true, NULL), code));

	code.setPipeProjection(&builder.createMultiStageProjection(pipe, mid));
}

void SQLGroupOps::Group::setUpDistinctGroupProjections(
		OpCodeBuilder &builder, OpCode &code,
		const SQLValues::CompColumnList &midKeyList,
		Projection **distinctProj) {

	const bool withDigest =
			!SQLValues::TupleDigester::isOrderingAvailable(midKeyList, false);

	SQLExprs::ExprRewriter &rewriter = builder.getExprRewriter();
	SQLExprs::ExprFactoryContext &factoryCxt = builder.getExprFactoryContext();

	SQLExprs::ExprRewriter::Scope scope(rewriter);
	rewriter.activateColumnMapping(factoryCxt);

	if (withDigest) {
		const uint32_t input = 0;
		rewriter.setIdOfInput(input, true);
	}

	{
		Projection &proj = builder.createProjectionByUsage(false);
		assert(proj.getProjectionCode().getType() == SQLOpTypes::PROJ_OUTPUT);

		proj.getProjectionCode().setKeyList(&midKeyList);
		code.setMiddleProjection(&proj);
	}

	if (withDigest) {
		rewriter.setIdProjected(true);
	}
	rewriter.setInputProjected(true);

	Projection &mid = builder.createProjectionByUsage(false);

	std::pair<Projection*, Projection*> projections =
			builder.arrangeProjections(code, false, true, distinctProj);
	assert(projections.second != NULL && *distinctProj != NULL);

	Projection &pipe = builder.createPipeFinishProjection(
			*projections.first, *projections.second);

	code.setPipeProjection(&builder.createMultiStageProjection(pipe, mid));
	code.setFinishProjection(NULL);
}


void SQLGroupOps::GroupDistinct::compile(OpContext &cxt) const {
	OpPlan &plan = cxt.getPlan();

	OpCodeBuilder builder(OpCodeBuilder::ofContext(cxt));

	const Projection *proj = getCode().getPipeProjection();

	SQLOpUtils::ExprRefList distinctList = builder.getDistinctExprList(*proj);
	const SQLType::AggregationPhase aggrPhase = getCode().getAggregationPhase();

	const SQLValues::CompColumnList *keyList = getCode().findKeyColumnList();
	const util::Set<uint32_t> *keySet = NULL;
	if (keyList != NULL) {
		keySet = &SQLExprs::ExprRewriter::compColumnListToSet(
			cxt.getAllocator(), *keyList);
	}

	const uint32_t outAttributes = OpCodeBuilder::resolveOutputProjection(
			*proj, NULL).getCode().getAttributes();
	const bool idSingle =
			((outAttributes & SQLExprs::ExprCode::ATTR_GROUPING) == 0);

	const OpNode *baseNode = &plan.getParentInput(0);
	for (SQLOpUtils::ExprRefList::const_iterator it = distinctList.begin();
			it != distinctList.end(); ++it) {
		const size_t index = it - distinctList.begin();

		OpNode &uniqNode = plan.createNode(SQLOpTypes::OP_GROUP);
		{
			OpCode code = getCode();
			code.setKeyColumnList(&builder.createDistinctGroupKeyList(
					distinctList, index, false, idSingle));
			code.setPipeProjection(&builder.createDistinctGroupProjection(
					distinctList, index, false, aggrPhase));
			uniqNode.setCode(code);
		}
		uniqNode.addInput(plan.getParentInput(1));

		OpNode *groupNode;
		if (aggrPhase == SQLType::AGG_PHASE_ADVANCE_PIPE) {
			groupNode = &uniqNode;
		}
		else {
			groupNode = &plan.createNode(SQLOpTypes::OP_GROUP);
			{
				OpCode code = getCode();
				code.setKeyColumnList(&builder.createDistinctGroupKeyList(
						distinctList, index, true, idSingle));
				code.setPipeProjection(&builder.createDistinctGroupProjection(
						distinctList, index, true, aggrPhase));
				code.setAggregationPhase(SQLType::AGG_PHASE_ALL_PIPE);
				groupNode->setCode(code);
			}
			groupNode->addInput(uniqNode);
		}

		OpNode &mergeNode =
				plan.createNode(SQLOpTypes::OP_GROUP_DISTINCT_MERGE);
		{
			OpCode code = getCode();
			code.setKeyColumnList(&builder.createDistinctMergeKeyList());
			code.setPipeProjection(&builder.createDistinctMergeProjection(
					distinctList, index, *proj, aggrPhase, keySet));
			mergeNode.setCode(code);
		}
		mergeNode.addInput(*baseNode);
		mergeNode.addInput(*groupNode);

		baseNode = &mergeNode;
	}
	plan.linkToAllParentOutput(*baseNode);
}


void SQLGroupOps::GroupDistinctMerge::execute(OpContext &cxt) const {
	typedef SQLValues::ValueUtils ValueUtils;
	typedef SQLValues::Types::Long IdType;

	TupleListReader &reader1 = cxt.getReader(0);
	TupleListReader &reader2 = cxt.getReader(1);
	TupleListReader &nextIdReader = cxt.getReader(0, 1);

	const SQLValues::CompColumn &idKeyColumn = getIdKeyColumn();
	const TupleColumn &column1 = idKeyColumn.getTupleColumn1();
	const TupleColumn &column2 = idKeyColumn.getTupleColumn2();

	SQLOpUtils::ProjectionRefPair emptyProjs;
	const SQLOps::Projection &proj = getProjections(emptyProjs);

	if (!reader1.exists()) {
		return;
	}

	MergeContext &mergeCxt = prepareMergeContext(cxt);

	int64_t id1 = ValueUtils::readCurrentValue<IdType>(reader1, column1);

	int64_t id2 = -1;
	if (reader2.exists()) {
		id2 = ValueUtils::readCurrentValue<IdType>(reader2, column2);
	}

	int64_t nextId = -1;
	do {
		if (!nextIdReader.exists()) {
			break;
		}
		else if (!mergeCxt.nextReaderStarted_) {
			nextIdReader.next();
			mergeCxt.nextReaderStarted_ = true;
			if (!nextIdReader.exists()) {
				break;
			}
		}
		nextId = ValueUtils::readCurrentValue<IdType>(nextIdReader, column1);
	}
	while (false);

	for (;;) {
		if (cxt.checkSuspended()) {
			return;
		}

		if (id2 < 0) {
			emptyProjs.first->project(cxt);
		}
		else {
			assert(id1 == id2);
			if (mergeCxt.merged_) {
				emptyProjs.second->project(cxt);
			}
			else {
				proj.project(cxt);
				mergeCxt.merged_ = true;
			}
		}

		bool stepping;
		{
			if (id2 >= 0) {
				reader2.next();
			}

			int64_t id;
			if (reader2.exists()) {
				id = ValueUtils::readCurrentValue<IdType>(reader2, column2);
				assert(id >= 0);
				stepping = (id != id2);
			}
			else {
				id = -1;
				stepping = true;
			}
			id2 = id;
		}

		if (!stepping && id1 != nextId) {
			continue;
		}

		reader1.next();
		if (!reader1.exists()) {
			assert(nextId < 0);
			break;
		}
		mergeCxt.merged_ = false;

		if (nextIdReader.exists()) {
			assert(nextId >= 0);
			nextIdReader.next();
		}
		else {
			assert(false);
		}

		id1 = nextId;
		if (!nextIdReader.exists()) {
			nextId = -1;
			continue;
		}

		nextId = ValueUtils::readCurrentValue<IdType>(nextIdReader, column1);
		assert(nextId >= 0);
	}
}

const SQLValues::CompColumn&
SQLGroupOps::GroupDistinctMerge::getIdKeyColumn() const {
	return getCode().getKeyColumnList().front();
}

const SQLOps::Projection& SQLGroupOps::GroupDistinctMerge::getProjections(
		SQLOpUtils::ProjectionRefPair &emptyProjs) const {
	const Projection *totalProj = getCode().getPipeProjection();
	assert(totalProj->getProjectionCode().getType() ==
			SQLOpTypes::PROJ_MULTI_STAGE);

	const Projection &mainProj = totalProj->chainAt(0);
	const Projection &totalEmptyProj = totalProj->chainAt(1);
	assert(totalEmptyProj.getProjectionCode().getType() ==
			SQLOpTypes::PROJ_MULTI_STAGE);

	emptyProjs.first = &totalEmptyProj.chainAt(0);
	emptyProjs.second = &totalEmptyProj.chainAt(1);

	return mainProj;
}

SQLGroupOps::GroupDistinctMerge::MergeContext&
SQLGroupOps::GroupDistinctMerge::prepareMergeContext(OpContext &cxt) const {
	if (cxt.getResource(0).isEmpty()) {
		cxt.getResource(0) = ALLOC_UNIQUE(cxt.getAllocator(), MergeContext);
	}
	return cxt.getResource(0).resolveAs<MergeContext>();
}


SQLGroupOps::GroupDistinctMerge::MergeContext::MergeContext() :
		merged_(false),
		nextReaderStarted_(false) {
}


void SQLGroupOps::GroupBucketHash::execute(OpContext &cxt) const {
	BucketContext &bucketCxt = prepareContext(cxt);
	cxt.setUpExprContext();

	const uint32_t outCount = cxt.getOutputCount();
	TupleListReader &reader = cxt.getReader(0);
	SQLValues::TupleListReaderSource readerSrc(reader);

	for (; reader.exists(); reader.next()) {
		if (cxt.checkSuspended()) {
			return;
		}

		const int64_t digest = bucketCxt.digester_(readerSrc);
		const uint64_t outIndex = static_cast<uint64_t>(digest) % outCount;

		bucketCxt.getOutput(outIndex).project(cxt);
	}
}

SQLGroupOps::GroupBucketHash::BucketContext&
SQLGroupOps::GroupBucketHash::prepareContext(OpContext &cxt) const {
	util::AllocUniquePtr<void> &resource = cxt.getResource(0);
	if (!resource.isEmpty()) {
		return resource.resolveAs<BucketContext>();
	}

	util::StackAllocator &alloc = cxt.getAllocator();
	resource = ALLOC_UNIQUE(
			alloc, BucketContext,
			alloc, cxt.getVarContext(), getCode().getKeyColumnList());

	BucketContext &bucketCxt = resource.resolveAs<BucketContext>();

	SQLOpUtils::ExpressionListWriter::Source writerSrc(
			cxt, *getCode().getPipeProjection(), false, NULL, false, NULL,
			SQLExprs::ExprCode::INPUT_READER);

	const uint32_t outCount = cxt.getOutputCount();
	for (uint32_t i = 0; i < outCount; i++) {
		writerSrc.setOutput(i);
		bucketCxt.writerList_.push_back(
				ALLOC_NEW(alloc) SQLOpUtils::ExpressionListWriter(writerSrc));
	}

	return bucketCxt;
}


SQLGroupOps::GroupBucketHash::BucketContext::BucketContext(
		util::StackAllocator &alloc, SQLValues::VarContext &varCxt,
		const SQLValues::CompColumnList &keyList) :
		writerList_(alloc),
		digester_(alloc, createBaseDigester(varCxt, keyList)) {
}

inline SQLOpUtils::ExpressionListWriter::ByGeneral
SQLGroupOps::GroupBucketHash::BucketContext::getOutput(uint64_t index) {
	return SQLOpUtils::ExpressionListWriter::ByGeneral(
			*writerList_[static_cast<size_t>(index)]);
}

SQLValues::TupleDigester
SQLGroupOps::GroupBucketHash::BucketContext::createBaseDigester(
		SQLValues::VarContext &varCxt,
		const SQLValues::CompColumnList &keyList) {
	const bool orderingAvailable =
			(keyList.empty() ? false : keyList.front().isOrdering());
	return SQLValues::TupleDigester(
			keyList, &varCxt, &orderingAvailable, false, false, false);
}


void SQLGroupOps::Union::compile(OpContext &cxt) const {
	if (tryCompileHashPlan(cxt)) {
		return;
	}
	else if (tryCompileEmptyPlan(cxt)) {
		return;
	}

	OpPlan &plan = cxt.getPlan();
	OpCode code = getCode();

	const uint32_t inCount = cxt.getInputCount();
	const SQLType::UnionType unionType = code.getUnionType();

	const bool sorted = (unionType != SQLType::UNION_ALL);
	const bool unique = (sorted && unionType != SQLType::END_UNION);
	const bool singleDistinct =
			(inCount == 1 && unionType == SQLType::UNION_DISTINCT);
	OpNode &node = (singleDistinct ?
			plan.getParentOutput(0) :
			plan.createNode(toOperatorType(unionType)));

	OpCodeBuilder builder(OpCodeBuilder::ofContext(cxt));

	if (sorted) {
		code.setKeyColumnList(&resolveKeyColumnList(
				cxt, builder, code.findKeyColumnList(), false));
	}

	std::pair<Projection*, Projection*> projections =
			builder.arrangeProjections(code, false, true, NULL);
	if (sorted) {
		OpCodeBuilder::removeUnificationAttributes(*projections.first);
		if (projections.second != NULL) {
			OpCodeBuilder::removeUnificationAttributes(*projections.second);
		}
	}

	for (uint32_t i = 0; i < inCount; i++) {
		if (sorted) {
			OpNode &subNode = plan.createNode(SQLOpTypes::OP_SORT);
			OpCode subCode;

			SQLExprs::ExprRewriter &rewriter = builder.getExprRewriter();
			SQLExprs::ExprRewriter::Scope scope(rewriter);
			rewriter.activateColumnMapping(builder.getExprFactoryContext());
			rewriter.setMappedInput(i, 0);

			subCode.setKeyColumnList(&code.getKeyColumnList());

			std::pair<Projection*, Projection*> subProjections;
			if (singleDistinct) {
				subProjections = projections;
			}
			else {
				Projection &proj = builder.createIdenticalProjection(true, i);
				OpCodeBuilder::removeUnificationAttributes(proj);
				subProjections.first = &proj;
			}
			subCode.setPipeProjection(subProjections.first);
			subCode.setFinishProjection(subProjections.second);

			if (unique) {
				subCode.setSubLimit(1);
			}

			subNode.setCode(subCode);
			subNode.addInput(plan.getParentInput(i));

			node.addInput(subNode);
		}
		else {
			node.addInput(plan.getParentInput(i));
		}
	}

	if (!singleDistinct) {
		code.setPipeProjection(projections.first);
		code.setFinishProjection(projections.second);

		node.setCode(code);
		plan.linkToAllParentOutput(node);
	}
}

bool SQLGroupOps::Union::tryCompileHashPlan(OpContext &cxt) const {
	OpPlan &plan = cxt.getPlan();

	OpCodeBuilder builder(OpCodeBuilder::ofContext(cxt));

	SQLOpTypes::Type hashOpType;
	const SQLValues::CompColumnList *keyList;
	if (!checkHashPlanAcceptable(
			cxt, builder, getCode(), &hashOpType, &keyList)) {
		return false;
	}

	OpNode &hashNode = plan.createNode(hashOpType);
	{
		OpCode code = getCode();
		OpNode &node = hashNode;

		code.setKeyColumnList(keyList);

		const uint32_t inCount = cxt.getInputCount();
		for (uint32_t i = 0; i < inCount; i++) {
			node.addInput(plan.getParentInput(i));
		}
		std::pair<Projection*, Projection*> projections =
				builder.arrangeProjections(code, false, true, NULL);
		assert(projections.second == NULL);

		util::Vector<Projection*> subProjList(cxt.getAllocator());
		for (uint32_t i = 0; i < inCount; i++) {
			Projection &subProj = builder.createIdenticalProjection(false, i);
			subProj.getPlanningCode().setOutput(i + 1);
			subProjList.push_back(&subProj);
		}

		SQLOps::Projection &proj = builder.createMultiOutputProjection(
				*projections.first, *subProjList.front());
		for (uint32_t i = 1; i < inCount; i++) {
			proj.addChain(*subProjList[i]);
		}
		code.setPipeProjection(&proj);

		node.setCode(code);

		plan.linkToAllParentOutput(node);
	}

	OpNode &nonHashNode = plan.createNode(SQLOpTypes::OP_UNION);
	{
		OpCode code = getCode();
		OpNode &node = nonHashNode;

		SQLOps::OpConfig &config = builder.createConfig();
		const SQLOps::OpConfig *srcConfig = getCode().getConfig();
		if (srcConfig != NULL) {
			config = *srcConfig;
		}
		config.set(SQLOpTypes::CONF_UNION_HASH_LIMIT, 1);
		if (code.getLimit() >= 0) {
			config.set(SQLOpTypes::CONF_LIMIT_INHERITANCE, 1);
		}
		code.setConfig(&config);

		const uint32_t inCount = cxt.getInputCount();
		for (uint32_t i = 0; i < inCount; i++) {
			node.addInput(hashNode, i + 1);
		}
		node.setCode(code);

		plan.linkToAllParentOutput(node);
	}

	return true;
}

bool SQLGroupOps::Union::tryCompileEmptyPlan(OpContext &cxt) const {
	const uint32_t inCount = cxt.getInputCount();
	if (inCount > 2 || !cxt.isAllInputCompleted()) {
		return false;
	}

	for (uint32_t i = 0; i < inCount; i++) {
		if (cxt.getInputBlockCount(i) > 0) {
			return false;
		}
	}

	OpPlan &plan = cxt.getPlan();

	ColumnTypeList outTypeList(cxt.getAllocator());
	OpCodeBuilder::getColumnTypeListByColumns(
			cxt.getOutputColumnList(0), outTypeList);

	OpCodeBuilder builder(OpCodeBuilder::ofContext(cxt));
	builder.setUpNoopNodes(plan, outTypeList);

	return true;
}

bool SQLGroupOps::Union::checkHashPlanAcceptable(
		OpContext &cxt, OpCodeBuilder &builder, const OpCode &code,
		SQLOpTypes::Type *hashOpType,
		const SQLValues::CompColumnList **keyList) {
	*hashOpType = SQLOpTypes::END_OP;
	*keyList = NULL;

	if (code.getLimit() < 0) {
		return false;
	}

	if (SQLOps::OpConfig::resolve(
			SQLOpTypes::CONF_UNION_HASH_LIMIT, code.getConfig()) > 0) {
		return false;
	}

	const uint32_t inCount = cxt.getInputCount();
	if (inCount > 2) {
		return false;
	}

	const SQLOpTypes::Type foundOpType =
			findHashOperatorType(code.getUnionType(), inCount);
	if (foundOpType == SQLOpTypes::END_OP) {
		return false;
	}

	if (code.getOffset() > 0) {
		return false;
	}

	const SQLValues::CompColumnList &resolvedKeyList = resolveKeyColumnList(
			cxt, builder, code.findKeyColumnList(), true);
	if (!checkKeySimple(resolvedKeyList)) {
		return false;
	}

	for (uint32_t i = 0; i < inCount; i++) {
		if (!checkInputSimple(cxt.getInputColumnList(i), resolvedKeyList)) {
			return false;
		}
	}

	if (!checkOutputSimple(
			code.getPipeProjection(), code.getFinishProjection())) {
		return false;
	}

	const int64_t memLimit = SQLOps::OpConfig::resolve(
			SQLOpTypes::CONF_WORK_MEMORY_LIMIT, code.getConfig());
	const uint64_t threshold =
			static_cast<uint64_t>(memLimit < 0 ? 32 * 1024 * 1024 : memLimit) / 2;
	if (estimateHashMapSize(cxt, resolvedKeyList, code.getLimit()) > threshold) {
		return false;
	}

	*hashOpType = foundOpType;
	*keyList = &resolvedKeyList;
	return true;
}

SQLOpTypes::Type SQLGroupOps::Union::toOperatorType(
		SQLType::UnionType unionType) {
	switch (unionType) {
	case SQLType::UNION_ALL:
		return SQLOpTypes::OP_UNION_ALL;
	case SQLType::UNION_DISTINCT:
		return SQLOpTypes::OP_UNION_DISTINCT_MERGE;
	case SQLType::UNION_INTERSECT:
		return SQLOpTypes::OP_UNION_INTERSECT_MERGE;
	case SQLType::UNION_EXCEPT:
		return SQLOpTypes::OP_UNION_EXCEPT_MERGE;
	default:
		assert(unionType == SQLType::END_UNION);
		return SQLOpTypes::OP_UNION_COMPENSATE_MERGE;
	}
}

SQLOpTypes::Type SQLGroupOps::Union::findHashOperatorType(
		SQLType::UnionType unionType, uint32_t inCount) {
	switch (unionType) {
	case SQLType::UNION_ALL:
		break;
	case SQLType::UNION_DISTINCT:
		return SQLOpTypes::OP_UNION_DISTINCT_HASH;
	case SQLType::UNION_INTERSECT:
		if (inCount != 2) {
			break;
		}
		return SQLOpTypes::OP_UNION_INTERSECT_HASH;
	case SQLType::UNION_EXCEPT:
		return SQLOpTypes::OP_UNION_EXCEPT_HASH;
	default:
		assert(unionType == SQLType::END_UNION);
		break;
	}
	return SQLOpTypes::END_OP;
}

const SQLValues::CompColumnList& SQLGroupOps::Union::resolveKeyColumnList(
		OpContext &cxt, OpCodeBuilder &builder,
		const SQLValues::CompColumnList *src, bool withAttributes) {
	if (!withAttributes && src != NULL) {
		return *src;
	}

	SQLValues::CompColumnList &dest = builder.createCompColumnList();

	if (src == NULL) {
		const uint32_t input = 0;
		const uint32_t columnCount = cxt.getColumnCount(input);
		for (uint32_t i = 0; i < columnCount; i++) {
			SQLValues::CompColumn column;
			column.setColumnPos(i, true);
			column.setOrdering(false);
			dest.push_back(column);
		}
	}
	else {
		dest = *src;
	}

	if (!withAttributes) {
		return dest;
	}

	return builder.getExprRewriter().rewriteCompColumnList(
			builder.getExprFactoryContext(), dest, true, NULL);
}

bool SQLGroupOps::Union::checkKeySimple(
		const SQLValues::CompColumnList &keyList) {
	for (SQLValues::CompColumnList::const_iterator it = keyList.begin();
			it != keyList.end(); ++it) {
		if (!SQLValues::SummaryTupleSet::isDeepReaderColumnSupported(
				it->getType())) {
			return false;
		}
	}
	return true;
}

bool SQLGroupOps::Union::checkInputSimple(
		const TupleColumnList &columnList,
		const SQLValues::CompColumnList &keyList) {
	for (SQLValues::CompColumnList::const_iterator it = keyList.begin();
			it != keyList.end(); ++it) {
		const TupleColumnType inType =
				columnList[it->getColumnPos(util::TrueType())].getType();
		if (SQLValues::TypeUtils::isAny(inType) &&
				!SQLValues::TypeUtils::isAny(it->getType())) {
			return false;
		}
	}
	return true;
}

bool SQLGroupOps::Union::checkOutputSimple(
		const Projection *pipeProj, const Projection *finishProj) {
	if (finishProj != NULL) {
		return false;
	}

	if (OpCodeBuilder::isAggregationArrangementRequired(
			pipeProj, finishProj)) {
		return false;
	}

	if (pipeProj->getProjectionCode().getType() != SQLOpTypes::PROJ_OUTPUT) {
		return false;
	}

	return true;
}

uint64_t SQLGroupOps::Union::estimateHashMapSize(
		OpContext &cxt, const SQLValues::CompColumnList &keyList,
		int64_t tupleCount) {
	if (tupleCount <= 0) {
		return 0;
	}

	const uint32_t input = 0;
	const SQLOps::TupleColumnList &columnList = cxt.getInputColumnList(input);

	util::StackAllocator &alloc = cxt.getAllocator();
	SQLValues::ValueContext valueCxt(
			SQLValues::ValueContext::ofAllocator(alloc));

	SQLValues::SummaryTupleSet tupleSet(valueCxt, NULL);
	tupleSet.addReaderColumnList(columnList);
	tupleSet.addKeyList(keyList, (input == 0), false);
	tupleSet.setReaderColumnsDeep(true);
	tupleSet.setHeadNullAccesible(true);
	tupleSet.completeColumns();

	const size_t hashHeadSize = sizeof(uint64_t) * 2;
	const size_t hashValueSize = tupleSet.estimateTupleSize();

	return static_cast<uint64_t>(tupleCount) * (hashHeadSize + hashValueSize);
}


void SQLGroupOps::UnionAll::compile(OpContext &cxt) const {
	const uint32_t count = cxt.getInputCount();
	for (uint32_t i = 0; i < count; i++) {
		cxt.setReaderLatchDelayed(i);
	}

	Operator::compile(cxt);
}

void SQLGroupOps::UnionAll::execute(OpContext &cxt) const {
	for (;;) {
		uint32_t index;
		if (!cxt.findRemainingInput(&index)) {
			break;
		}

		TupleListReader &reader = cxt.getReader(index);

		if (reader.exists()) {
			WriterEntry &writer = prepareWriter(cxt, index);
			*cxt.getActiveReaderRef() = &reader;
			if (writer.first != NULL) {
				writer.first->applyContext(cxt);
				do {
					writer.first->write();
					reader.next();

					if (cxt.checkSuspended()) {
						return;
					}
				}
				while (reader.exists());
			}
			else {
				writer.second->updateProjectionContext(cxt);
				do {
					writer.second->project(cxt);
					reader.next();

					if (cxt.checkSuspended()) {
						return;
					}
				}
				while (reader.exists());
			}
		}

		cxt.popRemainingInput();
		cxt.releaseReaderLatch(index);
	}
}

SQLGroupOps::UnionAll::WriterEntry&
SQLGroupOps::UnionAll::prepareWriter(OpContext &cxt, uint32_t index) const {
	util::StackAllocator &alloc = cxt.getAllocator();

	util::AllocUniquePtr<void> &resource = cxt.getResource(0);
	if (resource.isEmpty()) {
		resource = ALLOC_UNIQUE(
				alloc, UnionAllContext, alloc, cxt.getInputCount());
	}
	UnionAllContext &unionCxt = resource.resolveAs<UnionAllContext>();

	WriterEntry *&writer = unionCxt.writerList_[index];
	do {
		if (writer != NULL) {
			break;
		}

		std::pair<ColumnTypeList, WriterEntry*> mapEntry(
				ColumnTypeList(alloc), static_cast<WriterEntry*>(NULL));
		ColumnTypeList &typeList = mapEntry.first;
		getInputColumnTypeList(cxt, index, typeList);

		WriterMap::iterator mapIt = unionCxt.writerMap_.find(typeList);
		if (mapIt != unionCxt.writerMap_.end()) {
			writer = mapIt->second;
			break;
		}

		const SQLExprs::ExprCode::InputSourceType srcType =
				SQLExprs::ExprCode::INPUT_READER_MULTI;
		OpCodeBuilder::Source builderSrc = OpCodeBuilder::ofContext(cxt);
		builderSrc.mappedInput_ = index;

		OpCodeBuilder builder(builderSrc);
		cxt.setUpProjectionFactoryContext(
				builder.getProjectionFactoryContext(), &index);

		SQLExprs::ExprFactoryContext &exprCxt =
				builder.getExprFactoryContext();
		exprCxt.setInputSourceType(0, srcType);
		
		Projection *planningProj;
		{
			SQLExprs::ExprRewriter &rewriter = builder.getExprRewriter();
			SQLExprs::ExprRewriter::Scope scope(rewriter);
			planningProj = &builder.rewriteProjection(
					*getCode().getPipeProjection(), NULL);
			OpCodeBuilder::removeUnificationAttributes(*planningProj);
		}

		exprCxt.setPlanning(false);
		Projection *proj = &builder.rewriteProjection(*planningProj, NULL);

		SQLOpUtils::ExpressionListWriter::Source writerSrc(
				cxt, *proj, false, NULL, false, NULL, srcType, &index);

		SQLOpUtils::ExpressionListWriter *writerBase =
				ALLOC_NEW(alloc) SQLOpUtils::ExpressionListWriter(writerSrc);
		if (!writerBase->isAvailable()) {
			writerBase = NULL;
			proj = &builder.rewriteProjection(*proj, NULL);
		}
		writer = ALLOC_NEW(alloc) WriterEntry(writerBase, proj);
		mapEntry.second = writer;
		unionCxt.writerMap_.insert(mapEntry);

		cxt.setUpExprContext();
	}
	while (false);

	cxt.getExprContext().setReader(
			0, &cxt.getReader(index), &cxt.getInputColumnList(index));

	return *writer;
}

void SQLGroupOps::UnionAll::getInputColumnTypeList(
		OpContext &cxt, uint32_t index, ColumnTypeList &typeList) {
	const TupleColumnList &columnList = cxt.getInputColumnList(index);
	for (TupleColumnList::const_iterator it = columnList.begin();
			it != columnList.end(); ++it) {
		typeList.push_back(it->getType());
	}
}

SQLGroupOps::UnionAll::UnionAllContext::UnionAllContext(
		util::StackAllocator &alloc, uint32_t inCount) :
		writerList_(inCount, NULL, alloc),
		writerMap_(alloc) {
}


SQLGroupOps::UnionMergeContext::UnionMergeContext(int64_t initialState) :
		state_(initialState),
		initialState_(initialState),
		topElemChecked_(false) {
}

SQLGroupOps::TupleHeapQueue
SQLGroupOps::UnionMergeContext::createHeapQueue(
		OpContext &cxt,
		const SQLValues::CompColumnList &keyColumnList,
		util::LocalUniquePtr<TupleHeapQueue::Element> *topElem) {
	util::StackAllocator &alloc = cxt.getAllocator();

	TupleGreater pred(
			alloc,
			SQLValues::TupleComparator(
					keyColumnList, &cxt.getVarContext(), false, true),
			cxt.getValueProfile());
	UniqTupleDigester digester(
			alloc,
			SQLValues::TupleDigester(
					keyColumnList, &cxt.getVarContext(),
					NULL, true, false, false),
			cxt.getValueProfile());

	TupleHeapQueue queue(pred, alloc);
	queue.setPredicateEmpty(pred.getBase().isEmpty(0));

	const uint32_t count = cxt.getInputCount();
	for (uint32_t i = 0; i < count; i++) {
		TupleListReader &reader = cxt.getReader(i);
		if (!reader.exists()) {
			continue;
		}

		queue.push(TupleHeapQueue::Element(
				ReadableTupleRef(alloc, reader, digester), i));
	}

	do {
		if (topElem == NULL) {
			break;
		}

		TupleListReader &reader = cxt.getReader(0, 1);
		if (!reader.exists()) {
			break;
		}

		if (!topElemChecked_) {
			reader.next();
			if (!reader.exists()) {
				break;
			}
			topElemChecked_ = true;
		}

		*topElem = UTIL_MAKE_LOCAL_UNIQUE(
				*topElem, TupleHeapQueue::Element,
				ReadableTupleRef(alloc, reader, digester), 0);
	}
	while (false);

	return queue;
}


template<typename Op>
void SQLGroupOps::UnionMergeBase<Op>::compile(OpContext &cxt) const {
	cxt.setAllInputSourceType(SQLExprs::ExprCode::INPUT_READER_MULTI);

	Operator::compile(cxt);
}

template<typename Op>
void SQLGroupOps::UnionMergeBase<Op>::execute(OpContext &cxt) const {
	const SQLValues::CompColumnList &keyColumnList =
			getCode().getKeyColumnList();

	util::AllocUniquePtr<void> &resource = cxt.getResource(0);
	if (resource.isEmpty()) {
		resource = ALLOC_UNIQUE(
				cxt.getAllocator(), UnionMergeContext, Op::toInitial(cxt, getCode()));
	}
	UnionMergeContext &unionCxt = resource.resolveAs<UnionMergeContext>();

	util::LocalUniquePtr<TupleHeapQueue::Element> topElem;
	util::LocalUniquePtr<TupleHeapQueue::Element> *topElemRef =
			(Op::InputUnique::VALUE ? NULL : &topElem);
	TupleHeapQueue queue =
			unionCxt.createHeapQueue(cxt, keyColumnList, topElemRef);

	const SQLOps::Projection *projection = getCode().getPipeProjection();
	TupleHeapQueue::Element *topElemPtr = topElem.get();
	MergeAction action(cxt, unionCxt, projection, &topElemPtr);

	queue.mergeUnique(action);
}

template<typename Op>
inline bool SQLGroupOps::UnionMergeBase<Op>::onTuple(
		int64_t &state, size_t ordinal) {
	static_cast<void>(state);
	static_cast<void>(ordinal);
	return false;
}

template<typename Op>
inline bool SQLGroupOps::UnionMergeBase<Op>::onFinish(
		int64_t &state, int64_t initialState) {
	static_cast<void>(state);
	static_cast<void>(initialState);
	return true;
}

template<typename Op>
inline bool SQLGroupOps::UnionMergeBase<Op>::onSingle(
		size_t ordinal, int64_t initialState) {
	static_cast<void>(ordinal);
	static_cast<void>(initialState);
	return true;
}

template<typename Op>
int64_t SQLGroupOps::UnionMergeBase<Op>::toInitial(
		OpContext &cxt, const OpCode &code) {
	static_cast<void>(cxt);
	static_cast<void>(code);
	return 0;
}


template<typename Op>
SQLGroupOps::UnionMergeBase<Op>::MergeAction::MergeAction(
		OpContext &cxt, UnionMergeContext &unionCxt,
		const SQLOps::Projection *projection,
		TupleHeapQueue::Element **topElemPtr) :
		cxt_(cxt),
		unionCxt_(unionCxt),
		projection_(projection),
		topElemPtr_(topElemPtr) {
}

template<typename Op>
inline bool SQLGroupOps::UnionMergeBase<Op>::MergeAction::operator()(
		const TupleHeapQueue::Element &elem, const util::FalseType&) {
	if (Op::onTuple(unionCxt_.state_, elem.getOrdinal())) {
		projection_->projectBy(cxt_, elem.getValue());
	}


	return true;
}

template<typename Op>
inline void SQLGroupOps::UnionMergeBase<Op>::MergeAction::operator()(
		const TupleHeapQueue::Element &elem, const util::TrueType&) {
	if (Op::onFinish(unionCxt_.state_, unionCxt_.initialState_)) {
		projection_->projectBy(cxt_, elem.getValue());
	}
}

template<typename Op>
inline bool SQLGroupOps::UnionMergeBase<Op>::MergeAction::operator()(
		const TupleHeapQueue::Element &elem, const util::TrueType&,
		const util::TrueType&) {
	return Op::onSingle(elem.getOrdinal(), unionCxt_.initialState_);
}

template<typename Op>
template<typename Pred>
inline bool SQLGroupOps::UnionMergeBase<Op>::MergeAction::operator()(
		const TupleHeapQueue::Element &elem, const util::TrueType&,
		const Pred &pred) {
	if (!Op::InputUnique::VALUE) {
		if (elem.getOrdinal() == 0 && *topElemPtr_ != NULL) {
			const bool subFinishable = pred(**topElemPtr_, elem);
			if (!(*topElemPtr_)->next()) {
				(*topElemPtr_) = NULL;
			}
			return subFinishable;
		}
	}

	return true;
}


inline bool SQLGroupOps::UnionIntersect::onTuple(int64_t &state, size_t ordinal) {
	static_cast<void>(ordinal);
	--state;
	return false;
}

inline bool SQLGroupOps::UnionIntersect::onFinish(
		int64_t &state, int64_t initialState) {
	const bool matched = (state == 0);
	state = initialState;
	return matched;
}

inline bool SQLGroupOps::UnionIntersect::onSingle(
		size_t ordinal, int64_t initialState) {
	static_cast<void>(ordinal);
	return (initialState <= 1);
}

inline int64_t SQLGroupOps::UnionIntersect::toInitial(
		OpContext &cxt, const OpCode &code) {
	static_cast<void>(code);
	return static_cast<int64_t>(cxt.getInputCount());
}


inline bool SQLGroupOps::UnionExcept::onTuple(int64_t &state, size_t ordinal) {
	if (ordinal != 0) {
		state = 1;
	}
	return false;
}

inline bool SQLGroupOps::UnionExcept::onFinish(
		int64_t &state, int64_t initialState) {
	static_cast<void>(initialState);

	const bool matched = (state == 0);
	state = 0;
	return matched;
}

inline bool SQLGroupOps::UnionExcept::onSingle(
		size_t ordinal, int64_t initialState) {
	static_cast<void>(initialState);
	return (ordinal == 0);
}


inline bool SQLGroupOps::UnionCompensate::onTuple(
		int64_t &state, size_t ordinal) {
	if (ordinal != 0) {
		return false;
	}
	state = 1;
	return true;
}

inline bool SQLGroupOps::UnionCompensate::onFinish(
		int64_t &state, int64_t initialState) {
	static_cast<void>(initialState);

	const bool matched = (state == 0);
	state = 0;
	return matched;
}


SQLGroupOps::UnionHashContext& SQLGroupOps::UnionHashContext::resolve(
		OpContext &cxt, const SQLValues::CompColumnList &keyList,
		const Projection &baseProj, const SQLOps::OpConfig *config) {

	util::AllocUniquePtr<void> &resource = cxt.getResource(0);
	if (resource.isEmpty()) {
		resource = ALLOC_UNIQUE(
				cxt.getAllocator(), UnionHashContext,
				cxt, keyList, baseProj, config);
	}

	return resource.resolveAs<UnionHashContext>();
}

SQLGroupOps::UnionHashContext::UnionHashContext(
		OpContext &cxt, const SQLValues::CompColumnList &keyList,
		const Projection &baseProj, const SQLOps::OpConfig *config) :
		alloc_(cxt.getAllocator()),
		memoryLimit_(resolveMemoryLimit(config)),
		memoryLimitReached_(false),
		topInputPending_(false),
		followingInputCompleted_(false),
		map_(0, MapHasher(), MapPred(), alloc_),
		inputEntryList_(alloc_),
		proj_(NULL),
		projTupleRef_(NULL) {
	const bool digestOrdering = checkDigestOrdering(cxt, keyList);

	initializeTupleSet(cxt, keyList, tupleSet_, digestOrdering);
	setUpInputEntryList(cxt, keyList, baseProj, inputEntryList_, digestOrdering);

	cxt.setUpExprInputContext(0);
	proj_ = &createProjection(cxt, baseProj, keyList, digestOrdering);
	projTupleRef_ = cxt.getExprContext().getSummaryTupleRef(0);
	assert(projTupleRef_ != NULL);
}

template<typename Op>
const SQLOps::Projection* SQLGroupOps::UnionHashContext::accept(
		TupleListReader &reader, const uint32_t index, InputEntry &entry) {
	const int64_t digest = entry.digester_(ReaderSourceType(reader));
	const std::pair<Map::iterator, Map::iterator> &range =
			map_.equal_range(digest);
	for (Map::iterator it = range.first;; ++it) {
		if (it == range.second) {
			if (memoryLimitReached_) {
				return &entry.subProj_;
			}
			MapEntry entry(
					SummaryTuple::create(*tupleSet_, reader, digest, index), 0);
			const bool acceptable = Op::onTuple(entry.second, index);
			*projTupleRef_ = map_.insert(
					range.second, std::make_pair(digest, entry))->second.first;
			if (!acceptable) {
				return NULL;
			}
			break;
		}

		if (entry.tupleEq_(
				ReaderSourceType(reader),
				SummaryTuple::Wrapper(it->second.first))) {
			if (!Op::onTuple(it->second.second, index)) {
				return NULL;
			}
			*projTupleRef_ = it->second.first;
			break;
		}
	}

	return proj_;
}

void SQLGroupOps::UnionHashContext::checkMemoryLimit() {
	if (memoryLimitReached_) {
		return;
	}
	memoryLimitReached_ = (alloc_.getTotalSize() >= memoryLimit_);
}

SQLGroupOps::UnionHashContext::InputEntry&
SQLGroupOps::UnionHashContext::prepareInput(
		OpContext &cxt, const uint32_t index) {
	InputEntry *entry = inputEntryList_[index];

	entry->subProj_.updateProjectionContext(cxt);
	proj_->updateProjectionContext(cxt);

	return *entry;
}

bool SQLGroupOps::UnionHashContext::isTopInputPending() {
	return topInputPending_;
}

void SQLGroupOps::UnionHashContext::setTopInputPending(bool pending) {
	topInputPending_ = pending;
}

bool SQLGroupOps::UnionHashContext::isFollowingInputCompleted(OpContext &cxt) {
	do {
		if (followingInputCompleted_) {
			break;
		}

		const uint32_t inCount = cxt.getInputCount();
		for (uint32_t i = 1; i < inCount; i++) {
			if (!cxt.isInputCompleted(i)) {
				return false;
			}
		}

		followingInputCompleted_ = true;
	}
	while (false);
	return followingInputCompleted_;
}

uint64_t SQLGroupOps::UnionHashContext::resolveMemoryLimit(
		const SQLOps::OpConfig *config) {
	const int64_t base = SQLOps::OpConfig::resolve(
			SQLOpTypes::CONF_WORK_MEMORY_LIMIT, config);
	return static_cast<uint64_t>(base < 0 ? 32 * 1024 * 1024 : base);
}

const SQLOps::Projection& SQLGroupOps::UnionHashContext::createProjection(
		OpContext &cxt, const Projection &baseProj,
		const SQLValues::CompColumnList &keyList, bool digestOrdering) {
	const uint32_t index = 0;

	const SQLExprs::ExprCode::InputSourceType srcType =
			SQLExprs::ExprCode::INPUT_SUMMARY_TUPLE;
	OpCodeBuilder::Source builderSrc = OpCodeBuilder::ofContext(cxt);
	builderSrc.mappedInput_ = index;
	builderSrc.inputUnified_ = true;

	OpCodeBuilder builder(builderSrc);
	cxt.setUpProjectionFactoryContext(
			builder.getProjectionFactoryContext(), &index);

	SQLExprs::ExprFactoryContext &exprCxt =
			builder.getExprFactoryContext();
	exprCxt.setInputSourceType(0, srcType);

	Projection *planningProj;
	{
		SQLExprs::ExprRewriter &rewriter = builder.getExprRewriter();
		SQLExprs::ExprRewriter::Scope scope(rewriter);
		planningProj = &builder.rewriteProjection(baseProj.chainAt(0), NULL);
		OpCodeBuilder::removeUnificationAttributes(*planningProj);
	}

	exprCxt.setPlanning(false);
	exprCxt.setArrangedKeyList(&keyList, !digestOrdering);
	Projection *proj = &builder.rewriteProjection(*planningProj, NULL);
	proj = &builder.rewriteProjection(*proj, NULL);

	cxt.setUpExprContext();
	return *proj;
}

void SQLGroupOps::UnionHashContext::initializeTupleSet(
		OpContext &cxt, const SQLValues::CompColumnList &keyList,
		util::LocalUniquePtr<SummaryTupleSet> &tupleSet,
		bool digestOrdering) {
	const uint32_t input = 0;

	tupleSet = UTIL_MAKE_LOCAL_UNIQUE(
			tupleSet, SummaryTupleSet, cxt.getValueContext(), NULL);

	{
		ColumnTypeList typeList(cxt.getAllocator());
		{
			const uint32_t count = cxt.getColumnCount(input);
			for (uint32_t i = 0; i < count; i++) {
				typeList.push_back(OpCodeBuilder::resolveUnifiedInputType(cxt, i));
			}
		}
		tupleSet->addReaderColumnList(typeList);
		tupleSet->addKeyList(keyList, (input == 0), false);
	}

	{
		const uint32_t count = cxt.getInputCount();
		for (uint32_t i = 0; i < count; i++) {
			tupleSet->addSubReaderColumnList(i, cxt.getInputColumnList(i));
		}
	}

	tupleSet->setReaderColumnsDeep(true);
	tupleSet->setHeadNullAccesible(true);
	tupleSet->setOrderedDigestRestricted(!digestOrdering);

	tupleSet->completeColumns();

	*cxt.getSummaryColumnListRef(input) = tupleSet->getReaderColumnList();
}

void SQLGroupOps::UnionHashContext::setUpInputEntryList(
		OpContext &cxt, const SQLValues::CompColumnList &keyList,
		const Projection &baseProj, InputEntryList &entryList,
		bool digestOrdering) {
	util::StackAllocator &alloc = cxt.getAllocator();

	SQLExprs::ExprFactoryContext factoryCxt(alloc);
	SQLExprs::ExprRewriter rewriter(alloc);

	const uint32_t digesterInput = 0;

	{
		const uint32_t count = cxt.getColumnCount(0);
		for (uint32_t i = 0; i < count; i++) {
			const TupleColumnType type =
					OpCodeBuilder::resolveUnifiedInputType(cxt, i);
			factoryCxt.setInputType(0, i, type);
			factoryCxt.setInputType(1, i, type);
		}
	}

	const uint32_t inCount = cxt.getInputCount();
	assert(inCount <= 2);
	for (uint32_t i = 0; i < inCount; i++) {
		{
			const uint32_t count = cxt.getColumnCount(0);
			for (uint32_t j = 0; j < count; j++) {
				factoryCxt.setInputType(
						0, j, cxt.getReaderColumn(i, j).getType());

				assert(SQLValues::TypeUtils::findPromotionType(
						factoryCxt.getInputType(0, j),
						factoryCxt.getInputType(1, j), true) ==
								factoryCxt.getInputType(1, j));
				assert(!SQLValues::TypeUtils::isAny(factoryCxt.getInputType(0, j)) ==
					!SQLValues::TypeUtils::isAny(factoryCxt.getInputType(1, j)));
			}
		}

		const SQLValues::CompColumnList &digesterKeyList =
				rewriter.rewriteCompColumnList(
						factoryCxt, keyList, false, &digesterInput, !digestOrdering);
		const SQLValues::CompColumnList &eqKeyList =
				rewriter.rewriteCompColumnList(
						factoryCxt, keyList, false, NULL, !digestOrdering);

		const Projection &subProj = baseProj.chainAt(i + 1);
		entryList.push_back(ALLOC_NEW(alloc) InputEntry(
				cxt, digesterKeyList, digestOrdering, eqKeyList, subProj));
	}
}

bool SQLGroupOps::UnionHashContext::checkDigestOrdering(
		OpContext &cxt, const SQLValues::CompColumnList &keyList) {
	if (!SQLValues::TupleDigester::isOrderingAvailable(keyList, true)) {
		return false;
	}

	const uint32_t inCount = cxt.getInputCount();
	for (uint32_t i = 0; i < inCount; i++) {
		for (SQLValues::CompColumnList::const_iterator it = keyList.begin();
				it != keyList.end(); ++it) {
			const TupleColumnType type = cxt.getReaderColumn(
					i, it->getColumnPos(util::TrueType())).getType();
			if (type != it->getType()) {
				return false;
			}
		}
	}

	return true;
}


SQLGroupOps::UnionHashContext::InputEntry::InputEntry(
		OpContext &cxt, const SQLValues::CompColumnList &digesterKeyList,
		bool digestOrdering, const SQLValues::CompColumnList &eqKeyList,
		const Projection &subProj) :
		digester_(
				cxt.getAllocator(),
				SQLValues::TupleDigester(
						digesterKeyList, &cxt.getVarContext(), &digestOrdering,
						false, false, false),
				cxt.getValueProfile()),
		tupleEq_(
				cxt.getAllocator(),
				SQLValues::TupleComparator(
						eqKeyList, &cxt.getVarContext(),
						false, true, false, !digestOrdering),
				cxt.getValueProfile()),
		subProj_(subProj) {
}


template<typename Op>
void SQLGroupOps::UnionHashBase<Op>::compile(OpContext &cxt) const {
	const uint32_t count = cxt.getInputCount();
	for (uint32_t i = 0; i < count; i++) {
		cxt.setReaderLatchDelayed(i);
	}

	Operator::compile(cxt);
}

template<typename Op>
void SQLGroupOps::UnionHashBase<Op>::execute(OpContext &cxt) const {
	UnionHashContext &hashCxt = UnionHashContext::resolve(
			cxt, getCode().getKeyColumnList(), *getCode().getPipeProjection(),
			getCode().getConfig());
	const bool topInputCascading = Op::TopInputCascading::VALUE;
	for (;;) {
		uint32_t index;
		bool onPending = false;
		if (!cxt.findRemainingInput(&index)) {
			if (!topInputCascading || !hashCxt.isTopInputPending()) {
				break;
			}
			index = 0;
			onPending = true;
		}

		bool pendingNext = false;
		do {
			if (topInputCascading && index == 0 && !(onPending &&
					hashCxt.isFollowingInputCompleted(cxt))) {
				pendingNext = true;
				break;
			}

			TupleListReader &reader = cxt.getReader(index);
			if (!reader.exists()) {
				break;
			}

			hashCxt.checkMemoryLimit();
			UnionHashContext::InputEntry &entry =
					hashCxt.prepareInput(cxt, index);
			do {
				const Projection *proj =
						hashCxt.accept<Op>(reader, index, entry);
				if (proj != NULL) {
					proj->project(cxt);
				}
				reader.next();

				if (cxt.checkSuspended()) {
					return;
				}
			}
			while (reader.exists());
		}
		while (false);

		cxt.popRemainingInput();
		cxt.releaseReaderLatch(index);

		if (topInputCascading && index == 0) {
			hashCxt.setTopInputPending(pendingNext);
		}

		if (onPending) {
			break;
		}
	}
}

template<typename Op>
bool SQLGroupOps::UnionHashBase<Op>::onTuple(int64_t &state, size_t ordinal) {
	static_cast<void>(state);
	static_cast<void>(ordinal);
	assert(false);
	return false;
}


inline bool SQLGroupOps::UnionDistinctHash::onTuple(
		int64_t &state, size_t ordinal) {
	static_cast<void>(ordinal);

	if (state != 0) {
		return false;
	}

	state = -1;
	return true;
}


inline bool SQLGroupOps::UnionIntersectHash::onTuple(
		int64_t &state, size_t ordinal) {

	if (state <= 0) {
		if (state == 0) {
			state = static_cast<int64_t>(ordinal) + 1;
		}
		return false;
	}
	else if (state == static_cast<int64_t>(ordinal) + 1) {
		return false;
	}

	state = -1;
	return true;
}


inline bool SQLGroupOps::UnionExceptHash::onTuple(
		int64_t &state, size_t ordinal) {

	if (ordinal > 0) {
		state = -1;
		return false;
	}
	else if (state != 0) {
		return false;
	}

	state = -1;
	return true;
}
