#include "craftinfo.h"

CraftInfo::CraftInfo() {}

CraftInfo CraftInfo::fromJson(const QJsonObject& craftInfoObject) {
	CraftInfo craftInfo;

	if (craftInfoObject.contains("name") && craftInfoObject["name"].isString()) {
		craftInfo.name = craftInfoObject["name"].toString();
	}

	if (craftInfoObject.contains("type") && craftInfoObject["type"].isString()) {
		craftInfo.type = craftInfoObject["type"].toString();
	}

	if (craftInfoObject.contains("rankUnused") && craftInfoObject["rankUnused"].isBool()) {
		craftInfo.rankUnused = craftInfoObject["rankUnused"].toBool();
	}

	if (craftInfoObject.contains("mrank") && craftInfoObject["mrank"].isDouble()) {
		craftInfo.mrank = craftInfoObject["mrank"].toInt();
	}

	if (craftInfoObject.contains("rank") && craftInfoObject["rank"].isDouble()) {
		craftInfo.rank = craftInfoObject["rank"].toInt();
	}

	return craftInfo;
}

int CraftInfo::getRank() {
	return rank;
}

int CraftInfo::getMrank() {
	return mrank;
}

bool CraftInfo::isRankUnused() {
	return rankUnused;
}

QString CraftInfo::getType() {
	return type;
}

QString CraftInfo::getName() {
	return name;
}



