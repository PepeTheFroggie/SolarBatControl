#define num_datasets 720

struct wp
{
  int8_t state;
  int powhist;
  int bvolthist;
  int8_t hour;
};

wp wp_arr[num_datasets];
uint16_t datasetpt = 0;

void clearwp()
{ 
  for(int i=0;i<num_datasets;i++) 
  {
    wp_arr[i].hour  = -1; 
    wp_arr[i].state = -1;
  }
}

void storewp(int inp, float bvolt, int8_t state, int8_t hour)
{
  wp_arr[datasetpt].hour = hour;
  wp_arr[datasetpt].state = state;
  wp_arr[datasetpt].powhist = inp;
  wp_arr[datasetpt].bvolthist = (int)(30.0*bvolt);
  datasetpt++;
  if (datasetpt >= num_datasets) datasetpt = 0;
}

void handleCmd() 
{
  String out;

  for (uint8_t i = 0; i < server.args(); i++)
  {
    if      (server.argName(i) == "on")   {IsActive = true;  lowbatinhibit = false;}
    else if (server.argName(i) == "of")   {IsActive = false; highbatinhibit= false;}
    else if (server.argName(i) == "chg")   ChgOnly  = true;  
    else if (server.argName(i) == "nrm")   ChgOnly  = false; 
    else if (server.argName(i) == "cnow") {ChgNow   = true;  highbatinhibit= false;}
    else if (server.argName(i) == "cnrm")  ChgNow   = false; 
    else if (server.argName(i) == "HIgen") HIlimit = constrain(server.arg(i).toInt(),100,500); //200
    else if (server.argName(i) == "LOgen") LOlimit = constrain(server.arg(i).toInt(),-50,250);   // 10   
    else if (server.argName(i) == "HIchg") CHGlimitHI = constrain(server.arg(i).toInt(),-250,100);   //  -10
    else if (server.argName(i) == "LOchg") CHGlimitLO = constrain(server.arg(i).toInt(),-500,-100); // -200    
    else if (server.argName(i) == "HIBat") HIBat = server.arg(i).toFloat();  
    else if (server.argName(i) == "LOBat") LOBat = server.arg(i).toFloat();   
  }
  
  out += "<html><br><br><center>\n";
    
  if (IsActive) 
    out += "<button onclick=\"window.location.href='/cmd?of\';\">Control is ON </button>\n";
  else               
    out += "<button onclick=\"window.location.href='/cmd?on\';\">Control is OFF</button>\n";
  out += "&emsp;\n";

  if (ChgOnly) 
    out += "<button onclick=\"window.location.href='/cmd?nrm\';\">Charge & WR</button>\n";
  else               
    out += "<button onclick=\"window.location.href='/cmd?chg\';\">Charge ONLY</button>\n";
  out += "&emsp;\n";

  if (ChgNow) 
    out += "<button onclick=\"window.location.href='/cmd?cnrm\';\">Normal Charge</button>\n";
  else               
    out += "<button onclick=\"window.location.href='/cmd?cnow\';\">Force Charge </button>\n";

  out += "<br><br>\n";
  out += "<form method=\"post\">\n";
  out += "Inverter limit:&emsp;\n";
  out += "<input type=\"number\" name=\"HIgen\" value=\"";
  out += HIlimit;
  out += "\">&emsp;\n";
  out += "<input type=\"number\" name=\"LOgen\" value=\"";
  out += LOlimit;
  out += "\">&emsp;\n";
  out += "<input type=\"submit\"><br>\n";
  out += "</form>\n";

  out += "<form method=\"post\">\n";
  out += "Charger limit:&emsp;\n";
  out += "<input type=\"number\" name=\"HIchg\" value=\"";
  out += CHGlimitHI;
  out += "\">&emsp;\n";
  out += "<input type=\"number\" name=\"LOchg\" value=\"";
  out += CHGlimitLO;
  out += "\">&emsp;\n";
  out += "<input type=\"submit\"><br>\n";
  out += "</form>\n";

  out += "<form method=\"post\">\n";
  out += "Battery limit:&emsp;\n";
  out += "<input type=\"number\" step=\".1\" name=\"HIBat\" value=\"";
  out += HIBat;
  out += "\">&emsp;\n";
  out += "<input type=\"number\" step=\".1\" name=\"LOBat\" value=\"";
  out += LOBat;
  out += "\">&emsp;\n";
  out += "<input type=\"submit\"><br>\n";
  out += "</form>\n";
  
  out += "<a href=\"/timing\">Time Interval</a><br><br>\n";

  out += "<a href=\"/\">Back</a>\n";
  out += "</center></html>";
  server.send(200, "text/html", out);    
}

void getGraph() 
{
  String out;
  char bvstr[10];
  
  out = "<html>\n";  
  out += "<head>\n";
  sprintf(bvstr,"%d",HttpRefreshInterval);
  out += "<meta http-equiv='refresh' content='";
  out += bvstr;
  out += "'/>\n";
  
  out += "<title>BatControl</title>\n";
  out += "</head>\n";
    
  out += "<body><center>\n";
  out += "<h1>Bat Control</h1>\n";

/*
  if(IsActive) out += "A \n";
  if(WRstate) out += "WR \n";
  if(CHGstate) out += "CHG \n";
  out += "&emsp;\n";
/**/
  if(ChgOnly) out += "CO &emsp;\n";
  if(ChgNow) out += "CN &emsp;\n";

  out += "<a href=\"/cmd\">Config</a>&emsp;\n";

  if(lowbatinhibit) out += "LowBatInhibit &emsp;\n";
  if(highbatinhibit) out += "HighBatInhibit &emsp;\n";
  
  sprintf(bvstr,"%.2f",batvolt);
  out += "Bat Volt: ";
  out += bvstr;
  out += "&emsp;\n";
  
  sprintf(bvstr,"%d",pwrsum);
  out += "AC Power: ";
  out += bvstr;
  out += "&emsp;\n";
  
  out += "<a href=\"/up\">Upload</a><br><br>\n";

  out += "<img src=\"/pwr.svg\" />\n";
  out += "</center></body>\n";
  out += "</html>\n";
  
  server.send(200, "text/html", out);
}

void timeIntervals() 
{
  String out;
  for (uint8_t i = 0; i < server.args(); i++)
  {
    if      (server.argName(i) == "ShelT") ShellyInterval = 1000UL * constrain(server.arg(i).toInt(),10,180);
    else if (server.argName(i) == "LoopT") LoopInterval   = 1000UL * constrain(server.arg(i).toInt(),5,60);    
    else if (server.argName(i) == "HttpT") HttpRefreshInterval =     constrain(server.arg(i).toInt(),5,60);    
  }
   
  out += "<html><br><br><center>\n";

  out += "<form method=\"post\">\n";
  out += "Shelly timing:&emsp;\n";
  out += "<input type=\"number\" name=\"ShelT\" value=\"";
  out += ShellyInterval/1000;
  out += "\"><br><br>\n";
  out += "Loop timing:&emsp;\n";
  out += "<input type=\"number\" name=\"LoopT\" value=\"";
  out += LoopInterval/1000;
  out += "\"><br><br>\n";
  out += "Web Refresh:&emsp;\n";
  out += "<input type=\"number\" name=\"HttpT\" value=\"";
  out += HttpRefreshInterval;
  out += "\"><br><br>\n";
  out += "<input type=\"submit\"><br>\n";
  out += "</form>\n";

  out += "<a href=\"/cmd\">Back</a>\n";
  out += "</center></html>";
  server.send(200, "text/html", out);
}

void getPwr() 
{
  char pwrstr[10];
  sprintf(pwrstr,"%d \n",pwrsum);
  server.send(200, "text/html", pwrstr);
}

void handleNotFound() 
{
  server.send(404, "text/plain", "Not Here");
}

#define fullH 300
#define halfH 150
#define Hfact 0.15
#define Hfact2 2*Hfact

void drawGraph() 
{
  String out;
  out.reserve(6000);
  uint8_t oldstate = 0; int oldstatei = 0;
  uint8_t oldwp = 0; int oldi = 0;
  int8_t oldhour = -1;
  char temp[300];

  out += "<svg viewBox=\"0 0 740 300\" xmlns=\"http://www.w3.org/2000/svg\">\n";
  out += "<rect x=\"10\" y=\"0\" width=\"720\" height=\"300\" stroke=\"silver\" fill=\"none\"/>\n";
  out += "<line x1=\"10\" y1=\"150\" x2=\"730\" y2=\"150\" stroke=\"black\"/>\n";
  // horiz lines
  {
    int linenum;
    linenum = halfH - (float)HIlimit*Hfact;
    sprintf(temp,"<line x1=\"10\" y1=\"%d\" x2=\"730\" y2=\"%d\" stroke=\"aqua\"/>\n",linenum,linenum);
    out += temp;
    linenum = halfH - (float)LOlimit*Hfact;
    sprintf(temp,"<line x1=\"10\" y1=\"%d\" x2=\"730\" y2=\"%d\" stroke=\"aqua\"/>\n",linenum,linenum);
    out += temp;
    linenum = halfH - (float)CHGlimitHI*Hfact;
    sprintf(temp,"<line x1=\"10\" y1=\"%d\" x2=\"730\" y2=\"%d\" stroke=\"yellow\"/>\n",linenum,linenum);
    out += temp;
    linenum = halfH - (float)CHGlimitLO*Hfact;
    sprintf(temp,"<line x1=\"10\" y1=\"%d\" x2=\"730\" y2=\"%d\" stroke=\"yellow\"/>\n",linenum,linenum);
    out += temp;
    linenum = fullH - 30.0 * (float)HIBat*Hfact2;
    sprintf(temp,"<line x1=\"10\" y1=\"%d\" x2=\"730\" y2=\"%d\" stroke=\"coral\"/>\n",linenum,linenum);
    out += temp;
    linenum = fullH - 30.0* (float)LOBat*Hfact2;
    sprintf(temp,"<line x1=\"10\" y1=\"%d\" x2=\"730\" y2=\"%d\" stroke=\"coral\"/>\n",linenum,linenum);
    out += temp;
  }

  // colored state band 
  #define cbpos 151
  for (int i=0;i<num_datasets;i++)
  {
    int8_t newstate;
    uint16_t pos = datasetpt + i;
    if (pos >= num_datasets) pos -= num_datasets;
    newstate = wp_arr[pos].state;

    if (newstate != oldstate)
    {
      if (oldstate == 1) 
      {  
        sprintf(temp,"<line x1=\"%d\" y1=\"298\" x2=\"%d\" y2=\"298\" stroke=\"crimson\" />\n",10+oldstatei,10+i);
        out += temp;
        temp[0] = 0; // clear temp string
      }
      else if (oldstate == 2) 
      {  
        sprintf(temp,"<line x1=\"%d\" y1=\"298\" x2=\"%d\" y2=\"298\" stroke=\"green\" />\n",10+oldstatei,10+i);
        out += temp;
        temp[0] = 0; // clear temp string
      }      
      oldstatei = i;
      oldstate = newstate; 
    }
  }
  if (oldstate == 1)
  {
    sprintf(temp,"<line x1=\"%d\" y1=\"298\" x2=\"%d\" y2=\"298\" stroke=\"crimson\" />\n",10+oldstatei,10+num_datasets);
    out += temp;      
  }
  else if (oldstate == 2)
  {
    sprintf(temp,"<line x1=\"%d\" y1=\"298\" x2=\"%d\" y2=\"298\" stroke=\"green\" />\n",10+oldstatei,10+num_datasets);
    out += temp;      
  }

  // time mark & color band
  for (int i=0;i<num_datasets;i++)
  {
    int8_t newhour;
    uint16_t pos = datasetpt + i;
    if (pos >= num_datasets) pos -= num_datasets;

    // time mark
    newhour = wp_arr[pos].hour;
    if ((newhour != oldhour) && (oldhour >= 0))
    {
      if ((newhour == 12) || (newhour == 0))  // 12, 24
        sprintf(temp,"<line x1=\"%d\" y1=\"174\" x2=\"%d\" y2=\"126\" stroke=\"black\" />\n",10+i,10+i);
      else if ((newhour == 6) || (newhour == 18)) // 6, 18
        sprintf(temp,"<line x1=\"%d\" y1=\"166\" x2=\"%d\" y2=\"134\" stroke=\"black\" />\n",10+i,10+i);
      else 
        sprintf(temp,"<line x1=\"%d\" y1=\"158\" x2=\"%d\" y2=\"142\" stroke=\"black\" />\n",10+i,10+i);
      out += temp;        
    }
    oldhour = newhour;
  }

  // line graph red
  out += "<polyline points=\"";
  for (int i=0;i<num_datasets;i++)
  {
    // datasetpt is now, show the past
    uint16_t pos = datasetpt + i;
    if (pos >= num_datasets) pos -= num_datasets;
    int powdata = (float)wp_arr[pos].powhist*Hfact;
    if (wp_arr[pos].state >= 0)
    {
      sprintf(temp,"%d,%d ",10+i,halfH-powdata);
      out += temp;
    }
  }
  out += "\" fill=\"none\" stroke=\"red\" />\n";
  
  // line graph blue
  out += "<polyline points=\"";
  for (int i=0;i<num_datasets;i++)
  {
    // datasetpt is now, show the past
    uint16_t pos = datasetpt + i;
    if (pos >= num_datasets) pos -= num_datasets;
    int powdata = (float)wp_arr[pos].bvolthist*Hfact2;
    if (wp_arr[pos].state >= 0)
    {
      sprintf(temp,"%d,%d ",10+i,fullH-powdata);
      out += temp;
    }
  }
  out += "\" fill=\"none\" stroke=\"blue\" />\n";

  out += "</svg>\n";
  
  server.send(200, "image/svg+xml", out);
}
