/*
 * Code for ENC28J60 Chip - Arduino Ethernet Module
 * This code uses the ethercard library
 * and covers some basic functions such as
 * HTTP POST and GET as well as how to set up
 * the ENC28J60 as a server. (listening mode for client requests)
 * When using as a server, test using some Rest Client
 * It is also helpful to perform a GET/POST request with a rest client then match up
 * the response with the Serial monitor for testing. 
 * NOTE: Parts of the Code have been commented out. Uncomment to run/test those parts
 */
#include <EtherCard.h>

#define ethCSpin 10 //put the digital pin that the CS is connected to

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 }; //this is an example but any other uniqe number can be used

// giving static ip address (must be unique on the LAN)
static byte myip[] = { 192,168,1,203 };

//server info
const char website[] PROGMEM = "INSERT WEBSITE ADDRESS HERE"; //give the host webpage name without http or www. or any extensions
// (e.g. google.com not http://google.com/?q=arduino)
#define PATH "/" //relative path from the host for webpage address
//if there is no extra path, leave it as it is or blank (whatever works)
char websiteIp[] = "ENTER WEBSITE IP";//alternatively an IP address can be used. website is replaced but PATH is still used if necessary

byte Ethernet::buffer[500]; //This ethernet buffer contains all the information necessary for data transfer like ip addresses and tcp incoming data
//this buffer is what takes up most of the global variable space so adjust it as necessary
BufferFiller bfill; //to access buffer fill functions

Stash stash; //This is a datatype in the EtherCard library which is sent in HTTP requests. I believe it operates as a linked list

int res = 100; //timing variable
byte session; //Unique SessionID for tcp send so that the correct response from the server can be identified.

//initializing the ethernet module to preset static ip address
void staticInit(){
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  ether.staticSetup(myip);
  ether.printIp("IP: ", ether.myip);
  res = 180; //reset init value
}


//initializing the ethernet module using dhcp (dynamic ip address allocation)
void dhcpInit(){  

   if (ether.begin(sizeof Ethernet::buffer, mymac, ethCSpin) == 0){ //initializing ethernet
      Serial.println( "Failed to access Ethernet controller");
    }
    
    if (!ether.dhcpSetup()){  //getting dynamically set IP using dhcp
      Serial.println("DHCP failed");
    }

    ether.printIp("IP:  ", ether.myip); //print ip
    ether.printIp("GW:  ", ether.gwip);  //print gateway ip
    ether.printIp("DNS: ", ether.dnsip); //print dns 

    if (!ether.dnsLookup(website)) //connect to webpage
      Serial.println("DNS failed");
/*  if you're using an IP address instead of a website, replace the previous 
 *  two lines with the following commented lines
 *  ether.parseIp(ether.hisip, websiteIp); //accesses the website Ip instead of looking for website dns
 *  ether.hisport = ENTER PORT NUMBER; //enter port number if required
*/
    ether.printIp("SRV: ", ether.hisip);  //server ip (will come up if connection is successfully established)
    res = 180; //reset init value
}

/*
 * probably a useless function but it allows you to select which initialization mode you want
 * in one place instead of scattered throughout the code wherever you initialize/reinitialize ethernet
 */

void etherInit(){
//  staticInit(); //generally used in server mode
  dhcpInit(); //generally used in client mode
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);//Setting baud rate for serial communication
  Serial.println("\n[Ethernet HTTP Requests]");

  //Initialize Ethernet: You can auto-get an IP address using DHCP or set up a local static ip(choose one and comment the other out)
  etherInit();
}

void loop() {
/*********SERVER************/
  /* this is commented out
//Operating on listening (server) mode. (listening for incoming packets and requests)
  if(etherServer()){
    ether.httpServerReply(serverReply());
  }
  
  delay(200);
 */
 
 /*******GET/POST*******/ 
  if (res > 220){
    etherInit();
  }
  res++;
  ether.packetLoop(ether.packetReceive());
  if (res == 200){
  //Making an HTTP POST Request
 // etherPost();
 
  //Making an HTTP GET Request
  etherGet();
  }
  httpReply();
  delay(100);
}

void etherPost(){

  ether.packetLoop(ether.packetReceive());//looking for response
  byte sd = stash.create(); //create stash
/* 
 * example of adding data to the stash:
 * you can add parameters and then the data for the parameters
 * or simply pass the information without any descriptors
 * by using a predefined variable or entering it directly
 * in the format required by the server/api you are connecting to
 * stash.print("data=");
 * stash.print("test");
 * stash.print("&action="); // '&' is given to denote another parameter and signify the continued sending of data to the server/api as i have come to understand it
 * stash.print("submit");
*/
  stash.print(VARIABLE NAME OR "PARAMETER/DATA");

  stash.save();

  //Display data that was sent for debugging purposes
  Serial.println(VARIABLE NAME OR "DATA");

  //generate the header with payload and pass the "stash descriptor" as an argument using "$H"
  //$D = decimal, $F = webpages. 
  //the usage of this is similar to datatype descriptors like %d used in scanf statements in C programming

  Stash::prepare(PSTR("POST $F HTTP/1.1" "\r\n"   //can be changed to HTTP/1.0 or whatever you are using
      "Host: $F" "\r\n" //format is "HTTP HEADER NAME: VALUE"
//      "Content-Type: " "\r\n" // "\r\n" separates the different fields in what you are sending
//      "Accept: " "\r\n"   //i've provided a few examples of header values which can be used as required
      "Content-Length: $D" "\r\n" 
//      "Connection: " "\r\n" //options for these headers could be found online and used as per the requirements
      "\r\n" //this is required to separate the header from the payload(stash data which you are sending)
      "$H"),
      PSTR(PATH), website, stash.size(), sd); //if required switch website with websiteIp and add any other variables which were used
  
  //send the packet - this also releases all stash buffers once done
  session = ether.tcpSend();
}

/*
 * The GET protocol is very similar to POST
 * The same sequence is used and hence documentation is for the most part ommitted
 * Please refer to etherPost function for comments on Stash and tcpSend/Receive
 */

void etherGet(){
 
  ether.packetLoop(ether.packetReceive());
  byte sd = stash.create();
  
  stash.save(); //Get function has no payload and hence stash is blank

  Stash::prepare(PSTR("GET $F HTTP/1.1" "\r\n"
      "Host: $F" "\r\n"
      "Content-Type: text" "\r\n" //adjust headers as per requirements
      "Accept: text/plain" "\r\n" //example headers
      "Content-Length: $D" "\r\n"
      "Connection: close" "\r\n"
      "\r\n"
      "$H"),
     PSTR(PATH), website, stash.size(), sd);

  session = ether.tcpSend();// (session is a global variable and can be reused both here and in Post function since both will not be simulatneously called/executed)
}

/*
 * function to get reply from HTTP POST OR GET REQUESTS (SAME REPLY PROCESS)
 * you can always modify it to return the local reply pointer to some global variable.
*/

void httpReply(){

  char* reply = ether.tcpReply(session); //getting pointer to the tcp reply in the buffer and assigning it to the global reply variable


  if (reply != 0){
      res = 0;
      Serial.println(" >>>REPLY recieved...");
      Serial.println(reply); //this prints the header reply
//at times the reply message may also come thereby making the next few lines redundant...observe the serial moniter and adjust the code to your case
/************************************************************************************************
******the following TWO LINES OF CODE (uncommented lines) may NOT be required for GET request****
************************************************************************************************/
//but I found the to be ESSENTIAL to receive the payload for POST request
      ether.packetReceive();//This line is extremely important since it lets the arduino know to wait for the message packet from the server
//The previous line may also be replaced by calling the tcp persist function given below, which keeps the tcp connection open indefinitely for any and all packets to be received.
      Serial.println(reply);//this prints the message packet

  //ether.persistTcpConnection(true); //This may be included to replace the previous TWO LINES OF CODE (uncommented lines) since the connection automatically waits for more tcp packets
  }
}


/*
 * ether server is used to receive incoming tcp packets 
 * here we just keep listening(on loop) and if we get a message
 * we print the message to the serial monitor and reply back to the client
 * i've set it up as int so we can monitor whether a request was received or not
*/

int etherServer(){
  word len = ether.packetReceive(); //checks for packets
  word pos = ether.packetLoop(len); //gets the pointer to the data in the buffer

  if(pos){    //accesses the incoming message and prints it to the serial monitor
    bfill = ether.tcpOffset();
    char* request = (char *) Ethernet::buffer + pos;
    Serial.println(request); 
    return 1; //true (request received)
  }
  else{
  return 0; //false (no meaningful packets received)
  }  
}

static word serverReply(){
  bfill = ether.tcpOffset(); //putting the response in the buffer
  bfill.emit_p(PSTR(
    "HTTP/1.1 200 OK" "\r\n"    //http headers and status
    "Content-Type: text/plain" "\r\n"
    "\r\n"
    "Server is Active")); //reply message (payload). adjust the reply as required
  return bfill.position(); //returns the position of the reply which then gets sent out when the function is called using httpServerReply function in ethercard library.
}


