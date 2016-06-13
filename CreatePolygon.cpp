//
//  CreatePolygon.cpp
//  gps_mm_zed
//
//  Created by alphasgr on 6/11/16.
//  Copyright © 2016 Gorker Alp Malazgirt. All rights reserved.
//

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

void CreatePoligon() {
  cout<<"Please enter the file link for extracting car route: "<<endl;
  string carroute_file;
  cin >> carroute_file;
  cout<<"Please enter the file link for poligon map template: "<<endl;
  string map_template_file;
  cin >> map_template_file;
  fstream file_route(carroute_file);
  fstream file_template(map_template_file);
  
  file_route.open(carroute_file,std::fstream::in);
  file_template.open(map_template_file,std::fstream::in);
  if (!file_template.is_open()) {
    cout<<"Template not found. Quitting.."<<endl;
    return;
  }
  if (!file_route.is_open()) {
    cout<<"carroute not found. Quitting.."<<endl;
    return;
  }
  long previouscarid=0;
  string templateString="";
  ofstream htmlout("maphtml.html");
  bool allIdsRead = false;
  while (!allIdsRead) { //check if all routes are read
    htmlout.open("maphtml.html");
    for(string fline; getline(file_template,fline);)
    {
      stringstream ssm(fline);
      long carid,routenumber;
      double carlat,carlong,routelat,routelong;
      while(ssm.good())
      {
        if (fline.find("<!--coordinates-->") > -1){
          //readroute and fill
          
          for(string rline; auto& eofcheck=getline(file_template,rline);)
          {
            stringstream ssc(fline);
            if(ssc.good())
            {
              ssc>>carid>>carlat>>carlong>>routelat>>routelong>>routenumber;
              string str = templateString;
              str += "{lat:"+to_string(routelat)+", lng:"+to_string(routelong)+"},\n";
              //{lat: 37.772, lng: -122.214},
              if(previouscarid==carid || previouscarid>=0){
                htmlout<<str;
              }
              else if (previouscarid != carid && previouscarid>=0) {
                //new file
                templateString = str;
                previouscarid = carid; //replace previous car id;
                break; //continues filling the html
              }
            }
            if (eofcheck.eof()){
              allIdsRead = true;
            }
          }
        }
        else {
          htmlout<<fline;
        }
      }
    }
    htmlout.close();
    string name = to_string(previouscarid);
    rename("maphtml",name.c_str());
  }
  
}