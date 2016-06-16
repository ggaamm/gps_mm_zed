//
//  main.cpp
//  gps_mm_zed
//
//  Created by Gorker Alp Malazgirt (-|>/-*-\<|-) on 6/1/16.
//
//libraries
////TODO: replace cout with debug info
////TODO: add counters for measuring execution
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <set>
#include <math.h>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <functional>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PI 3.14159265
#define R 6372795.477598
#define MAXIMUMDISTANCE
using namespace std;
static const double minLattitude = 40.95124;
static const double minLongtitude = 29.01658;
typedef pair<double,double> coordinatetype;
typedef pair<long,long> gridtype;

//mkfifo -m 666 fifo1.dat

struct st_map_point {
  coordinatetype point;
  gridtype grid_id;
  vector<coordinatetype>previous_point;
  vector<string> street_name;
  vector<double> street_id;
};
struct st_car_point {
  vector<coordinatetype> pointArray;
  vector<int> altitudeArray;
  long mobile_id;
  vector<string> directionArray;
  vector<coordinatetype> route;
};
gridtype FindGridId(coordinatetype mapcoordinate,
                    double minLattitude,double minLongtitude) {
  double dLat = (mapcoordinate.first * 1000000) - minLattitude*1000000 ;
  double dLong = (mapcoordinate.second * 1000000) - minLongtitude*1000000;
  //this division gridifies
  dLat /= 651.3; //#12.7207 #lattitute gridify
  dLong /= 937.7; //#9.157  #longtitude gridfy
  return make_pair(static_cast<long>(dLat),static_cast<long>(dLong));
}
//map datastructure
map<coordinatetype, st_map_point> coord_to_loc_struct;
map<long, st_car_point> car_id_to_car_struct;
map<gridtype,set<coordinatetype>> grid_to_coord;
map<long,vector<coordinatetype>> calculatedRoute;
void GenerateDirectionalPoints(vector<coordinatetype>& carpoints,string car_direction){
  ////a car is represented by three points second element is the car, add first and third
  double car_lattitute = carpoints[1].first;
  double car_longtitute = carpoints[1].second;
  //'G?NEY','KUZEY','KUZEYBATI','DO?U','KUZEYDO?U','BATI','G?NEYDO?U','G?NEYBATI'
  if (car_direction.compare("G?NEY")==0) { //south
    carpoints[0].first = car_lattitute - 0.000045;
    carpoints[0].second = car_longtitute;
    carpoints[2].first = car_lattitute + 0.000045;
    carpoints[2].second = car_longtitute;
  }
  else if (car_direction.compare("KUZEY")==0) { //north
    carpoints[0].first = car_lattitute + 0.000045;
    carpoints[0].second = car_longtitute;
    carpoints[2].first = car_lattitute - 0.000045;
    carpoints[2].second = car_longtitute;
  }
  else if(car_direction.compare("DO?U")==0){ //east
    carpoints[0].first = car_lattitute;
    carpoints[0].second = car_longtitute + 0.000060;
    carpoints[2].first = car_lattitute;
    carpoints[2].second = car_longtitute - 0.000060;
  }
  else if(car_direction.compare("BATI")==0){ //west
    carpoints[0].first = car_lattitute;
    carpoints[0].second = car_longtitute - 0.000060;
    carpoints[2].first = car_lattitute;
    carpoints[2].second = car_longtitute + 0.000060;
  }
  else if(car_direction.compare("KUZEYDO?U")==0){ //northeast
    carpoints[0].first = car_lattitute + 0.000027;
    carpoints[0].second = car_longtitute + 0.000048;
    carpoints[2].first = car_lattitute - 0.000027;
    carpoints[2].second = car_longtitute - 0.000048;
  }
  else if(car_direction.compare("KUZEYBATI")==0){ //north west
    carpoints[0].first = car_lattitute + 0.000027;
    carpoints[0].second = car_longtitute - 0.000048;
    carpoints[2].first = car_lattitute - 0.000027;
    carpoints[2].second = car_longtitute + 0.000048;
  }
  else if(car_direction.compare("G?NEYDO?U")==0){ //south east
    carpoints[0].first = car_lattitute - 0.000027;
    carpoints[0].second = car_longtitute + 0.000048;
    carpoints[2].first = car_lattitute + 0.000027;
    carpoints[2].second = car_longtitute - 0.000048;
  }
  else if(car_direction.compare("G?NEYBATI")==0){ //south west
    carpoints[0].first = car_lattitute - 0.000027;
    carpoints[0].second = car_longtitute - 0.000048;
    carpoints[2].first = car_lattitute + 0.000027;
    carpoints[2].second = car_longtitute + 0.000048;
  }
}
double haversine(coordinatetype p1, coordinatetype p2){
  auto start = std::chrono::system_clock::now();
  //call your function here
  
  double lat1 = p1.first*PI/180;
  double lat2 = p2.first*PI/180;
  double lon1 = p1.second*PI/180;
  double lon2 = p2.second*PI/180;
  double dlon = lon2 - lon1;
  double dlat = lat2 - lat1;
  double a = (sin(dlat/2))*(sin(dlat/2)) + cos(lat1) * cos(lat2) * (sin(dlon/2))*(sin(dlon/2));
  double c = 2 * atan2( sqrt(a), sqrt(1-a) );
  double d = R * c; //where R is the radius of the Earth
  auto stop = std::chrono::system_clock::now();
  std::chrono::duration<double, std::milli>
  time = stop - start;
  //cout << fixed << setprecision(6)
  //<< time.count() << "  ms for havesine"<<endl;
  return d;
}

double distancetolinefrompoint(coordinatetype p1,coordinatetype p2, coordinatetype carpoint) {
  auto start = std::chrono::system_clock::now();
  double lat1 = p1.first;
  double lat2 = p2.first;
  double long1 = p1.second;
  double long2 = p2.second;
  double carlat = carpoint.first;
  double carlong = carpoint.second;
  double dist = abs(((long2-long1)*carlat) - ((lat2-lat1) * carlong) + (lat2*long1) - (long2*lat1));
  dist /= sqrt(pow(long2-long1,2)+pow(lat2-lat1,2));
  auto stop = std::chrono::system_clock::now();
  std::chrono::duration<double, std::micro>
  time = stop - start;
  //cout << time.count() << "  ms for distance"<<endl;
  return dist;
}

void ReadCarAndPipe() {
  string car = "/Users/gorkeralp/Dropbox/Okul/Research/QPU/gps_data_csv/gpsdata1.csv";
  fstream file;
  //pipe related
  string path = "/Users/gorkeralp/Downloads/fifo1.dat";
  ofstream fifo_file;
  fifo_file.open(path);
  
  //setlinebuf(stdout);
  //unlink(path.c_str());
  //mkfifo(path.c_str(),S_IRWXU | S_IRWXG | S_IRWXO);
  file.open(car);
  
  string line; getline(file,line ); //this is for csv signature
  //cout<<line<<endl;
  cout<<"Reading Car Data..."<<endl;
  for(string line; getline(file,line ); )
  {
    stringstream ss(line);
    vector<string> result;
    while(ss.good())
    {
      string substr;
      getline( ss, substr, ',' );
      result.push_back( substr );
    }
    //result2 car id
    long car_id = stol(result[2]);
    double dLat = stod(result[5]);
    double dLong = stod(result[6]);
    pair <double,double> coordinate_pair = make_pair(dLat, dLong);
    if (car_id_to_car_struct.count(car_id)>0) { //car exists in the dict
      car_id_to_car_struct[car_id].altitudeArray.push_back(stod(result[7]));
      car_id_to_car_struct[car_id].directionArray.push_back(result[9]);
      car_id_to_car_struct[car_id].pointArray.push_back(coordinate_pair);
      //mobile id exists no need to insert
      //cout<<"inside"<<endl;
    }
    else { //car does not exist in the dict - insert it
      st_car_point carpoint;
      carpoint.altitudeArray.push_back(stod(result[7]));
      carpoint.directionArray.push_back(result[9]);
      carpoint.pointArray.push_back(coordinate_pair);
      carpoint.mobile_id = car_id;
      car_id_to_car_struct.insert(pair<long,st_car_point>(carpoint.mobile_id,carpoint));
      //send it over pipe here
    }
  }
  cout<<"Car data is read"<<endl;
  file.close();
  
  //send it over the pipe
  for (auto& car_map : car_id_to_car_struct) { //pair
    unsigned long int arraysize = car_map.second.pointArray.size(); //mobile id car size
    //cout<<arraysize<<endl;
    for (int i = 0; i < arraysize; i++) {
      fifo_file <<car_map.second.mobile_id <<"," // mobileid
      << car_map.second.pointArray[i].first <<"," //lat
      <<car_map.second.pointArray[i].second<<"," //long
      <<car_map.second.directionArray[i] //direction
      <<'\n';
    }
    //fifocar.second.mobile_id
  }
  //send all the data here
  /*
   double a = 40.34;
   double b = 34.34;
   int c = 4234234;
   for (int i = 0; i < 3; i++) {
   fifo_file <<a<<b<<c<<'\n';
   }
   */
  fifo_file <<"done"<<'\n';
  fifo_file.close();
  
  cout<<"Car data is read and sent to pipe"<<endl;
  
}

void ReadGPSAndCalculate() {
  string path = "/Users/gorkeralp/Downloads/fifo1.dat";
  string line;
  ifstream myfile;
  myfile.open(path);
  string d;
  ofstream resultFile;
  while (true) //keeps until termination
  {
    for(string line; getline(myfile,line ); )
    {
      if(line.compare("done")==0) return;
      
      stringstream ss(line);
      vector<string> result;
      while(ss.good())
      {
        string substr;
        getline( ss, substr, ',' );
        result.push_back( substr );
      }
      
        
      //result2 car id
      long car_id = stol(result[0]);
      double dLat = stod(result[1]);
      double dLong = stod(result[2]);
      string ddirection = result[3];
      //cout <<car_id<<" "<<dLat<<" "<<dLong<<" "<<ddirection<<endl;
      //calculate and return
      coordinatetype carpointfromgps = make_pair(dLat, dLong);
      vector<coordinatetype> generatedcarpoints(3);
      generatedcarpoints[1] = carpointfromgps;
      string direction = result[3];
      GenerateDirectionalPoints(generatedcarpoints, direction);
      map<int,pair<coordinatetype,double>> distanceMap;
      vector<double> distance={500,500,500};
      vector<gridtype> neighborGridPoints;
      for (int j=0;j<generatedcarpoints.size();j++) { //for each generated point
        //findgrid id
        gridtype carpointgrid_id = FindGridId(generatedcarpoints[j], minLattitude, minLongtitude);
        neighborGridPoints.push_back(carpointgrid_id);
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first+1,carpointgrid_id.second));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first-1,carpointgrid_id.second));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first+1,carpointgrid_id.second+1));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first-1,carpointgrid_id.second+1));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first,carpointgrid_id.second+1));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first,carpointgrid_id.second-1));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first+1,carpointgrid_id.second-1));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first-1,carpointgrid_id.second-1));
      }
      for (const auto& gridPoint : neighborGridPoints){ //for each grid from the neighbors
        const auto& gridpoint  = grid_to_coord.find(gridPoint);
        if (gridpoint == grid_to_coord.end())
          continue;//no elements in this grid,search in the neighbor point
        for (coordinatetype mappoint : gridpoint->second) //each map point in the grid
        {
          double dist0,dist1,dist2;
          for (const auto& prevPoint : coord_to_loc_struct[mappoint].previous_point) {
            if (prevPoint.first == -1 | prevPoint.second ==-1) {
              //send three of them
              
              dist0 = haversine(generatedcarpoints[0], mappoint); //if prevPoint is not known do haversine
              dist1 = haversine(generatedcarpoints[1], mappoint); //if prevPoint is not known do haversine
              dist2 = haversine(generatedcarpoints[2], mappoint); //if prevPoint is not known do haversine
              
            }
            else {
              dist0 = distancetolinefrompoint(mappoint,prevPoint,generatedcarpoints[0]);
              dist1 = distancetolinefrompoint(mappoint,prevPoint,generatedcarpoints[1]);
              dist2 = distancetolinefrompoint(mappoint,prevPoint,generatedcarpoints[2]);
            }
            if (distance[0] > dist0) {
              distance[0] = dist0;
              distanceMap[0] = make_pair(mappoint,dist0);
            }
            if (distance[1] > dist1) {
              distance[1] = dist1;
              distanceMap[1] = make_pair(mappoint,dist1);
            }
            if (distance[2] > dist2) {
              distance[2] = dist2;
              distanceMap[2] = make_pair(mappoint,dist2);
            }
          }
          //cout<<"prev point size:"<<coord_to_loc_struct[mappoint].previous_point.size()<<endl;
          //for each prev point, we can calculate line distance as well but of course there won't be previous for
          //all points
          //double dist = haversine(generatedcarpoints[j], mappoint); //we wanna do it for all the three points in a pipeline
        }
      }
      if (distance[0] <= 30 || distance[1] <= 30 || distance[3] <= 30){
        if(distanceMap[0].first == distanceMap[1].first ) { //all the points are aligned
          calculatedRoute[car_id].push_back(distanceMap[0].first);
          //car_id.second.route.push_back(distanceMap[0].first);
        }
        else if(distanceMap[0].first == distanceMap[2].first){
          calculatedRoute[car_id].push_back(distanceMap[0].first);
        }
        else if(distanceMap[1].first == distanceMap[2].first){
          calculatedRoute[car_id].push_back(distanceMap[1].first);
        }
        else {
          calculatedRoute[car_id].push_back(distanceMap[1].first);
        }
        unsigned long size = calculatedRoute[car_id].size();
        //print car route to file
        //cout <<car_id<<endl;
        ////representation carid,retrieved point,found point,routenumber
        string st_car_id ="/Users/gorkeralp/Downloads/";
        st_car_id += to_string(car_id);
        st_car_id += "_res.txt";
        resultFile.open(st_car_id,std::ios::app);
        resultFile <<car_id <<","
        <<setprecision(15)<<carpointfromgps.first<<","
        <<setprecision(15)<<carpointfromgps.second<<","
        <<setprecision(15)<<calculatedRoute[car_id][size-1].first<<","
        <<setprecision(15)<<calculatedRoute[car_id][size-1].second<<","
        <<size<<'\n';
        resultFile.close();
      }
    }
  }
}

void ReadMap() {
  cout << "Read the map and form the grids!\n";
  //string yol = "/Users/gorkeralp/Dropbox/Okul/Research/QPU/kadikoy_yol.json";
  string yol2 = "/Users/gorkeralp/Developer/json_gps/json_gps/newfile.txt";
  fstream file;
  file.open(yol2);
  cout<<"Reading Map Data..."<<endl;
  for(string line; getline(file,line ); )
  {
    stringstream ss(line);
    string street_name; string street_id;
    double dstreet_id=-1;
    if (ss.good()) { //get street name
      getline( ss, street_name, ',' );
    } else {break;}
    if (ss.good()) { //get street id
      getline( ss, street_id, ',' );
      dstreet_id = stod(street_id);
    } else {break;}
    //this starts at the each stree_id
    double prevLat=-1;double prevLong=-1;
    while(ss.good())
    {
      string sLong;
      string sLat;
      getline( ss, sLong, ',' ); //first longtide
      getline( ss, sLat, ',' ); //second lattitude
      //Get rid of the white space and parenthesis
      if (sLong.find(' ') != string::npos)
        sLong.replace(sLong.find(' '), 1,"");
      if (sLat.find(' ') != string::npos)
        sLat.replace(sLat.find(' '), 1,"");
      if (sLong.find('[') != string::npos)
        sLong.replace(sLong.find('['), 1,"");
      if (sLat.find('[') != string::npos)
        sLat.replace(sLat.find('['), 1,"");
      if (sLong.find(']') != string::npos)
        sLong.replace(sLong.find(']'), 1,"");
      if (sLat.find(']') != string::npos)
        sLat.replace(sLat.find(']'), 1,"");
      double dLat = stod(sLat);
      double dLong = stod(sLong);
      coordinatetype coordinate_pair = make_pair(dLat, dLong);
      gridtype grid_id = FindGridId(coordinate_pair,minLattitude,minLongtitude);
      //create the point
      if (coord_to_loc_struct.count(coordinate_pair)>0) { //point exist
        //gridId and coordinate pair exist, no need to update
        if(prevLat != -1 && prevLong !=-1){coord_to_loc_struct[coordinate_pair].previous_point.push_back(make_pair(prevLat, prevLong));}
        coord_to_loc_struct[coordinate_pair].street_id.push_back(dstreet_id);
        coord_to_loc_struct[coordinate_pair].street_name.push_back(street_name);
      } else { //coordinate does not exist, create it
        st_map_point gps_map_point;
        gps_map_point.grid_id = grid_id;
        gps_map_point.point = coordinate_pair;
        gps_map_point.previous_point.push_back(make_pair(prevLat, prevLong));
        gps_map_point.street_id.push_back(dstreet_id);
        gps_map_point.street_name.push_back(street_name);
        coord_to_loc_struct.insert(pair<coordinatetype, st_map_point>(coordinate_pair, gps_map_point));
      }
      if(grid_to_coord.count(grid_id)>0) { //grid exists
        grid_to_coord[grid_id].insert(coordinate_pair);
      } else { //grid doesnt exist
        set<coordinatetype> cp;
        cp.insert(coordinate_pair);
        grid_to_coord.insert(pair<gridtype,set<coordinatetype>>(grid_id,cp));
      }
      prevLat = coordinate_pair.first;
      prevLong = coordinate_pair.second;
    }
  }
  file.close();
  cout<<"Map Data is Read..."<<endl;
}

int main(int argc, const char * argv[]) {
  auto start = chrono::system_clock::now();
  pid_t pid = fork();
  if (pid == 0) {
    printf("This is the child process. My pid is %d and my parent's id is %d.\n", getpid(), getppid());
    ReadCarAndPipe();
  } else {
    printf("This is the parent process. My pid is %d and my parent's id is %d.\n", getpid(), pid);
    ReadMap();
    ReadGPSAndCalculate();
    auto stop = chrono::system_clock::now();
    std::chrono::duration<double, std::milli>
    time = stop - start;
    cout << fixed << setprecision(6)
    << time.count() << "  ms for havesine"<<endl;
  }

  return 0;
}
