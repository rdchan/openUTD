/* 
Friend Detector by Ricardo Oliveira, forked by Skickar 9/30/2018

A Node MCU microcontroller + mini bread board + 4 pin RGB LED to detect when devices belonging to a target are nearby.


The function of this code is to read nearby Wi-Fi traffic in the form of packets. These packets are compared
to a list of MAC addresses we wish to track, and if the MAC address of a packet matches one on the list, we turn 
on a colored LED that is linked to the user owning the device. For example, when my roommate comes home, the 
transmissions from his phone will be detected and cause the blue LED to turn on until his phone is no longer detected. 
It can detect more than one phone at a time, meaning if my phone (red) and my roommate's phone (blue) are both home, 
the LED will show purple. */

#include "./esppl_functions.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

//#include <vector>
#include <string>
#include <vector>
/*  Define you friend's list size here
 How many MAC addresses are you tracking?
 */
#define LIST_SIZE 5
#define BUFFER_SIZE 50
#define NUM_BUCKETS 256
#define hit_cutoff 20
#define deletions_per_iter 2
#define BACS_TIMEOUT 10000
//#define BACS_CUTOFF 3600000*6
#define BACS_CUTOFF 60000
#define cucket_cap 240


const char* ssid = "NSHSS";
const char* password = "lmaoscam";

int iter = 0;
uint8_t num_devices = 0;



void publish_init() {
  wifi_promiscuous_enable(false);
  wifi_set_channel(1);
  esppl_set_channel(1);
   wifi_station_connect();
  //WiFi.mode(WIFI_AP_STA);

}


bool establishConnection() {
  publish_init();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Connecting..");
  }
  Serial.println("Connected!\n");
  //SendWebRequest();
  return true;
}

void SendWebRequest() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    Serial.println("Going to send data");
    HTTPClient http;  //Declare an object of class HTTPClient
    //c11cf209bbcb2a8577bc94f238782f21  ECSW 1.365
    //c1beb01773ad90447c4ad050b8ad55d6 RHSW 2.008
    String str = "http://openutd.us-east-2.elasticbeanstalk.com/_set?id=c11cf209bbcb2a8577bc94f238782f21&data=";
    str += num_devices;
    http.begin(str);
    Serial.println(str);
    //Serial.println(num_devices);
    int httpCode = http.GET();                                                                  //Send the request
 
    if (httpCode > 0) { //Check the returning code
      String payload = http.getString();   //Get the request response payload
      Serial.println(payload);                     //Print the response payload 
    } else { 
      Serial.println("http error :(");
    }
 
    http.end();   //Close connection
  }

  WiFi.printDiag(Serial);
  esppl_init(cb);
    esppl_sniffing_start();

}

class mac{
  public:
  uint8_t addr[6];
  mac(uint8_t arr[6]){
    for(int i=0;i<6;i++){
      addr[i] = arr[i];
    }
  }

  bool operator==(const mac &rhs) const{
    for(int i=0;i<6;i++){
      if(addr[i]!=rhs.addr[i]) return false;
    }
    return true;
  }

  bool operator!=(const mac &rhs) const{
    return !((*this)==rhs);
  }
  
  void print() {
    Serial.printf("%d:%d:%d:%d:%d:%d",addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
  }
};

std::vector<mac> cuckmacs;

class Bacs {
  public:
    std::vector<mac> addrs;
    std::vector<unsigned long> touts;
    std::vector<unsigned long> age;
    
    void addMac(mac toAdd){
      for(int i=0;i<addrs.size();i++){
        if(addrs.at(i) == toAdd){
          touts.at(i) = millis();
          return;
        }
      }
      addrs.push_back(toAdd);
      touts.push_back(millis());
      age.push_back(touts.at(touts.size()-1));
    }

    void degrade(){
      for(int i=0;i<touts.size();i++){
        if(millis()-touts.at(i)>=BACS_TIMEOUT){
          cuckmacs.push_back(addrs.at(i));
          touts.erase(touts.begin() + i);
          addrs.erase(addrs.begin() + i);
          age.erase(age.begin() + i);
          i--;
        }if(millis()-age.at(i)>=BACS_CUTOFF){
          for(int j=0;j<cuckmacs.size();j++){
            if(cuckmacs.at(j) == addrs.at(i)){
              cuckmacs.erase(cuckmacs.begin() + j);
              break;
            }
          }
        }
      }
    }

    int numberOfBacs() { return addrs.size(); }

    bool hasMac(mac ncheese) {
      for(int i = 0; i < addrs.size(); i++) {
        if(addrs.at(i) == ncheese && touts.at(i)>=BACS_CUTOFF) {
          return true;
        }
      }
     // Serial.println("bacs doesn't have dude\n");
      return false;
    }
    
};

class Buckets{
  private:
  uint8_t cycle = 0;
  uint8_t die_rate;
  uint8_t cutoff;
  public:
    Buckets(uint8_t cut, uint8_t die) {
      cutoff = cut;
      die_rate = die;
    }
    uint8_t buckets[NUM_BUCKETS];
    void increment(uint8_t index) {
      if(buckets[index] < 255) {
        buckets[index]++;
       // Serial.println("+++");
      } 
    }
    void degrade() {
      
      cycle += die_rate;
     // Serial.printf("\t%d", cycle);
      for(int i = cycle; i < die_rate+cycle; i++) {
        if(buckets[i%NUM_BUCKETS] > 0) {
          buckets[i%NUM_BUCKETS]--;
       //   Serial.printf("\t--- %d", i%NUM_BUCKETS);
            //Serial.println("---");
        }
      }
    }
    bool aboveThreshold(uint8_t index) {
      return buckets[index] >= cutoff;
    }

    bool belowCap(uint8_t index) {
      return buckets[index] <= cucket_cap;
    }
};

/*
 * This is your friend's MAC address list
 Format it by taking the mac address aa:bb:cc:dd:ee:ff 
 and converting it to 0xaa,0xbb,0xcc,0xdd,0xee,0xff
 */
/*
 * This is your friend's name list
 * put them in the same order as the MAC addresses
 */

template<typename DataType>
bool vectorHas(std::vector<DataType> vect, DataType entry){
  for(int i=0;i<vect.size();i++){
    if(vect.at(i)==entry) return true;
  }
  //Serial.println("vector doesn't have dude\n");
  return false;
  
}

//std::vector<std::string> macs;

Buckets buckets(20, 4); //16, \n 32
Buckets cuckets(20, 8);
//Bacs bacs;


// start variables package - Skickar 2018 hardware LED for NodeMCU on mini breadboard //
void setup() { 
  //delay(500);
  Serial.begin(115200);
   WiFi.persistent(false);
    //  delay(1000);
      WiFi.printDiag(Serial);
      Serial.println("Starting up...");
  /*while(WiFi.status() != WL_CONNECTED) {
    yield();
    delay(100);
    yield();
    Serial.println("connecting...\n");
  }
  Serial.println("Connected!");
  */
  pinMode(D5, OUTPUT); // sets the pins to output mode
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);

  //establishConnection();
  esppl_init(cb);

/*
  std::vector<int> v;
  v.push_back(22);
  v.push_back(-5);
  delay(2000);*/
  /*if(vectorHas(v, 22) && !vectorHas(v, 0)) Serial.println("yes");
  else //Serial.println("um");
  delay(1000);*/
}


/* You cannot use a time delay here to keep the LED on, so will need to use ratio of 
detected packets to overall packets to keep LED on for longer. If you try to use a 
delay to keep the light on for long enough to be useful, the watchdog timer kills the 
process and it dies */
int cooldown = 0; /* This variable will be a cooldown timer to keep the LED on for longer, we'll set it to 1000 if we
detect a packet from a device with a MAC address on the list, and then keep the LED on till we get 1000 packets that 
are NOT from any device on the list. */
void red() {
digitalWrite(D5, HIGH); }  // Turn ON the red LED
void blue() {
digitalWrite(D7, HIGH); } // Turn ON the blue LED
void green() {
digitalWrite(D6, HIGH); } // Turn ON the green LED
void turnoff() {
    digitalWrite(D7, LOW), digitalWrite(D5, LOW), digitalWrite(D6, LOW); 
}

/* end exparimental variable package */

bool maccmp(uint8_t *mac1, uint8_t *mac2) {
  
  for (int i=0; i < ESPPL_MAC_LEN; i++) {
    if (mac1[i] != mac2[i]) {
      return false;
    }
  }
  return true;
}

/*std::string macFromInfo(esppl_frame_info *info) {
  char* boi;
  sprintf(boi, "%s:%s:%s:%s:%s:%s",String(info->sourceaddr[0], HEX).c_str(), String(info->sourceaddr[1], HEX).c_str(), String(info->sourceaddr[2], HEX).c_str(), String(info->sourceaddr[3], HEX).c_str(), String(info->sourceaddr[4], HEX).c_str(), String(info->sourceaddr[5], HEX).c_str());
  return std::string(boi);
}*/
void cb(esppl_frame_info *info) {
  // Serial.printf("%s",&macFromInfo(info));
   //Serial.printf("\n\thit at MAC: %s:%s:%s:%s:%s:%s",String(info->sourceaddr[0], HEX).c_str(), String(info->sourceaddr[1], HEX).c_str(), String(info->sourceaddr[2], HEX).c_str(), String(info->sourceaddr[3], HEX).c_str(), String(info->sourceaddr[4], HEX).c_str(), String(info->sourceaddr[5], HEX).c_str());
   buckets.increment(info->sourceaddr[5]);
   if(buckets.aboveThreshold(info->sourceaddr[5])) {
      cuckets.increment(info->sourceaddr[4]);
      /*if(cuckets.aboveThreshold(info->sourceaddr[4])) {
        // deprecated attempt to shorten things and eliminate phantom macs mac(info->sourceaddr);
        if(!vectorHas(cuckmacs, mac(info->sourceaddr))){
          if(!bacs.hasMac(mac(info->sourceaddr))){
          //Serial.println("pushing back..");
          //mac(info->sourceaddr).print();
          //Serial.println("\n");
            cuckmacs.push_back(mac(info->sourceaddr));
          } else {
           //Serial.println(" but, it's in bacs, so no add");
          }
          bacs.addMac(mac(info->sourceaddr));
        }
      }*/
   
  /*for (int i=0; i<LIST_SIZE; i++) {
    if (maccmp(info->sourceaddr, friendmac[i]) || maccmp(info->receiveraddr, friendmac[i])) {
      Serial.printf("\n%s is here! :)", friendnanme[i].c_str());
      cooldown = 1000; // here we set it to 1000 if we detect a packet that matches our list
      if (i == 1){
        blue();} // Here we turn on the blue LED until turnoff() is called
        else {
          red();} // Here we turn on the RED LED until turnoff is called. We can also use if i == 0, or another index
    }

      else { // this is for if the packet does not match any we are tracking
        if (cooldown > 0) {
          cooldown--; } //subtract one from the cooldown timer if the value of "cooldown" is more than one
          else { // If the timer is at zero, then run the turnoff function to turn off any LED's that are on.
        
        turnoff(); } } } 
        */
 }
}


void loop() { 


  esppl_sniffing_start();
  
  while (true) {
    iter++;
    
    Serial.println(iter);
    Serial.println("\n");
    if(iter == 250) {  
      iter = 0;
      //esppl_sniffing_stop();
      delay(1000);

      bool successful_connection = establishConnection();
     if(successful_connection) {
        SendWebRequest();
      } else {
        Serial.println("Couldn't connect :(");
      }
        esppl_init(cb);
    }
        num_devices = 0;
        buckets.degrade();
        cuckets.degrade();
         // bacs.degrade();
      

       // num_devices = cuckmacs.size();
         //Serial.printf("\ndevices: %d",cuckmacs.size());
          /*for(int i = 0; i < cuckmacs.size(); i++) {
              cuckmacs.at(i).print();
              Serial.printf("\t");
          }*/
          

         //Serial.println("\n");
          for(int i = 0; i < NUM_BUCKETS; i++) {
            if(cuckets.aboveThreshold(i) && cuckets.belowCap(i)) {
              Serial.printf("%d (%d)\t",i,cuckets.buckets[i]);
              num_devices++;
            }
          }
          //  Serial.println("\n");
          
          Serial.printf("\ndevices: %d\n",num_devices);
        
          
          /*for(int i = 0; i < NUM_BUCKETS; i++) {
            if(macs[i] != "") {
              Serial.printf("\n\t%s",macs[i].c_str());
            }
          }*/
          
      for (int i = ESPPL_CHANNEL_MIN; i <= ESPPL_CHANNEL_MAX; i++ ) {
        esppl_set_channel(i);
        while (esppl_process_frames());
      }
      esppl_set_channel(1);
  }  
}
