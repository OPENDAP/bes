The nlohmann JSON library is used by the CMR module and _should_ be used
by all the code we have that reads or writes JSON. In the past we used
rapidjson, but its API is very hard to use.

Both rapidjson and nlohmann are header libraries.

The nlohmann library can be found at https://github.com/nlohmann/json/releases.

jhrg 5/11/26
