#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>
#include <ArduinoJson.h>
static uint32_t ticks=0;
static uint32_t millis() { return ticks; }
static void delay(uint32_t ms) { ticks+=ms; }
#include "../race_track/ota_stream.h"

// Model the supported SDK's TLS behavior: -1 when a later record has not arrived,
// even though the connection is alive. A record is released every 7 milliseconds.
struct FragmentedClient {
  std::string data; size_t position=0, fragment=64; uint32_t started=ticks, interval=7;
  bool keepOpen=false;
  int available() {
    const size_t released=std::min(data.size(), (size_t(uint32_t(ticks-started)/interval)+1)*fragment);
    return int(released-position);
  }
  bool connected() { return keepOpen || position<data.size(); }
  int read(uint8_t* out,size_t count) {
    int ready=available(); if(ready<=0)return -1;
    count=std::min(count,size_t(ready)); memcpy(out,data.data()+position,count); position+=count; return int(count);
  }
};
struct LegacyReader {
  FragmentedClient& client;
  int read() { uint8_t c; return client.read(&c,1)>0?c:-1; }
  size_t readBytes(char* out,size_t n) { size_t k=0; for(;k<n;++k){int c=read();if(c<0)break;out[k]=char(c);}return k; }
};
static void check(bool ok,const char* name) { if(!ok){fprintf(stderr,"FAIL: %s\n",name);exit(1);} printf("PASS: %s\n",name); }
static std::string fixture() {
  return "{\"body\":\""+std::string(6000,'x')+"\",\"tag_name\":\"v0.1.23\",\"assets\":[{\"name\":\"firmware.bin\",\"browser_download_url\":\"https://github.com/Ayxrion/race-track/releases/download/v0.1.23/firmware.bin\"}]}";
}
int main() {
  StaticJsonDocument<256> filter; filter["tag_name"]=true; filter["assets"][0]["name"]=true; filter["assets"][0]["browser_download_url"]=true;
  DynamicJsonDocument doc(4096);
  FragmentedClient old{fixture()}; LegacyReader legacy{old};
  auto error=deserializeJson(doc,legacy,DeserializationOption::Filter(filter));
  check(error==DeserializationError::IncompleteInput,"old SDK read behavior reproduces IncompleteInput between TLS records");
  ticks=0; FragmentedClient client{fixture()}; OtaJsonReader<FragmentedClient> reader(client);
  error=deserializeJson(doc,reader,DeserializationOption::Filter(filter));
  check(!error && std::string(doc["tag_name"].as<const char*>())=="v0.1.23","fragmented release metadata parses with the production reader");
  check(std::string(doc["assets"][0]["name"].as<const char*>())=="firmware.bin","asset selection survives a long ignored release body");
  check(!reader.timedOut && !reader.tooLarge && ticks>0,"reader waits for delayed packets without claiming EOF");
  ticks=0; FragmentedClient truncated{fixture().substr(0,100)}; OtaJsonReader<FragmentedClient> cut(truncated);
  error=deserializeJson(doc,cut,DeserializationOption::Filter(filter));
  check(error==DeserializationError::IncompleteInput,"actual truncation remains an error");
  ticks=0; FragmentedClient stalled{""}; stalled.keepOpen=true; OtaJsonReader<FragmentedClient> idle(stalled,65536,20,100);
  check(idle.read()==-1 && idle.timedOut && ticks==20,"stalled connection has a bounded idle timeout");
  ticks=0; FragmentedClient large{fixture()}; OtaJsonReader<FragmentedClient> capped(large,100);
  error=deserializeJson(doc,capped,DeserializationOption::Filter(filter));
  check(error && capped.tooLarge && capped.received()==100,"metadata size is bounded");
  ticks=0; FragmentedClient slow{fixture()}; slow.fragment=1; OtaJsonReader<FragmentedClient> deadline(slow,65536,20,40);
  error=deserializeJson(doc,deadline,DeserializationOption::Filter(filter));
  check(error && deadline.timedOut && ticks==40,"trickling data cannot extend the overall deadline");
  ticks=0xfffffff0u; FragmentedClient wrap{fixture()}; OtaJsonReader<FragmentedClient> wrapping(wrap);
  error=deserializeJson(doc,wrapping,DeserializationOption::Filter(filter));
  check(!error,"timeouts tolerate millis rollover");
}
