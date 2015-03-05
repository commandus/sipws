#include "RegistryEntry.h"


RegistryEntry::RegistryEntry() : o(entrynop), u(""), k(""), cn(""), description(""), image("")
{
}

RegistryEntry::RegistryEntry(EOperation o, const std::string &u, const std::string &k, const std::string &cn, const std::string &description, const std::string &image)
{
	this->o = o;
	this->u = u;
	this->k = k;
	this->cn = cn;
	this->description = description;
	this->image = image;
}

RegistryEntry::RegistryEntry(const Json::Value &value)
{
	if ((&value == NULL) || (!value.isObject()))
	{
		this->o = entrynop;
		this->u = "";
		this->k = "";
		return;
	}

	Json::Value o = value.get("o", "");
	this->o = parseOperation(o.asString());
	this->u = value.get("u", "").asString();
	this->k = value.get("k", "").asString();
	this->cn = value.get("cn", "").asString();
	this->description = value.get("description", "").asString();
	this->image = value.get("image", "").asString();
}

RegistryEntry::~RegistryEntry()
{
}

Json::Value RegistryEntry::toJson()
{
	Json::Value r(Json::objectValue);
	if (o != entrynop)
		r["o"] = toString(o);
	if ((&u != NULL) && (!u.empty()))
		r["u"] = u;
	if ((&k != NULL) && (!k.empty()))
		r["k"] = k;
	if ((&cn != NULL) && (!cn.empty()))
		r["cn"] = cn;
	if ((&description != NULL) && (!description.empty()))
		r["description"] = description;
	return r;
}
