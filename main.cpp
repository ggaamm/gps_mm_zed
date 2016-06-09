//
//  main.cpp
//  gps_mm_zed
//
//  Created by Gorker Alp Malazgirt (-|>/-*-\<|-) on 6/1/16.
//
//libraries
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
#define PI 3.14159265
#define R 6372795.477598
using namespace std;
static const double minLattitude = 40.95124;
static const double minLongtitude = 29.01658;
typedef pair<double,double> coordinatetype;
typedef pair<long,long> gridtype;
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
void GenerateDirectionalPoints(vector<coordinatetype>& carpoints,string car_direction){
  ////a car is represented by three points second element is the car, add first and third
  double car_lattitute = carpoints[1].first;
  double car_longtitute = carpoints[1].second;
  //'G?NEY','KUZEY','KUZEYBATI','DO?U','KUZEYDO?U','BATI','G?NEYDO?U','G?NEYBATI'
  if (car_direction.compare("G?NEY")) { //south
    carpoints[0].first = car_lattitute - 0.000045;
    carpoints[0].second = car_longtitute;
    carpoints[2].first = car_lattitute + 0.000045;
    carpoints[2].second = car_longtitute;
  }
  else if (car_direction.compare("KUZEY")) { //north
    carpoints[0].first = car_lattitute + 0.000045;
    carpoints[0].second = car_longtitute;
    carpoints[2].first = car_lattitute - 0.000045;
    carpoints[2].second = car_longtitute;
  }
  else if(car_direction.compare("DO?U")){ //east
    carpoints[0].first = car_lattitute;
    carpoints[0].second = car_longtitute + 0.000060;
    carpoints[2].first = car_lattitute;
    carpoints[2].second = car_longtitute - 0.000060;
  }
  else if(car_direction.compare("BATI")){ //west
    carpoints[0].first = car_lattitute;
    carpoints[0].second = car_longtitute - 0.000060;
    carpoints[2].first = car_lattitute;
    carpoints[2].second = car_longtitute + 0.000060;
  }
  else if(car_direction.compare("KUZEYDO?U")){ //northeast
    carpoints[0].first = car_lattitute + 0.000027;
    carpoints[0].second = car_longtitute + 0.000048;
    carpoints[2].first = car_lattitute - 0.000027;
    carpoints[2].second = car_longtitute - 0.000048;
  }
  else if(car_direction.compare("KUZEYBATI")){ //north west
    carpoints[0].first = car_lattitute + 0.000027;
    carpoints[0].second = car_longtitute - 0.000048;
    carpoints[2].first = car_lattitute - 0.000027;
    carpoints[2].second = car_longtitute + 0.000048;
  }
  else if(car_direction.compare("G?NEYDO?U")){ //south east
    carpoints[0].first = car_lattitute - 0.000027;
    carpoints[0].second = car_longtitute + 0.000048;
    carpoints[2].first = car_lattitute + 0.000027;
    carpoints[2].second = car_longtitute - 0.000048;
  }
  else if(car_direction.compare("G?NEYBATI")){ //south west
    carpoints[0].first = car_lattitute - 0.000027;
    carpoints[0].second = car_longtitute - 0.000048;
    carpoints[2].first = car_lattitute + 0.000027;
    carpoints[2].second = car_longtitute + 0.000048;
  }
}
double haversine(coordinatetype p1, coordinatetype p2){
  double lat1 = p1.first*PI/180;
  double lat2 = p2.first*PI/180;
  double lon1 = p2.second*PI/180;
  double lon2 = p2.second*PI/180;
  double dlon = lon2 - lon1;
  double dlat = lat2 - lat1;
  double a = (sin(dlat/2))*(sin(dlat/2)) + cos(lat1) * cos(lat2) * (sin(dlon/2))*(sin(dlon/2));
  double c = 2 * atan2( sqrt(a), sqrt(1-a) );
  double d = R * c; //where R is the radius of the Earth
  return d;
}

int main(int argc, const char * argv[]) {
  cout << "Read the map and form the grids!\n";
  string yol = "/Users/gorkeralp/Dropbox/Okul/Research/QPU/kadikoy_yol.json";
  string yol2 = "/Users/gorkeralp/Developer/json_gps/json_gps/newfile.txt";
  string car = "/Users/gorkeralp/Dropbox/Okul/Research/QPU/gps_data_csv/gps data1.csv";
  //read map file from user
  //read car file from user
  ifstream file(yol2);
  //read the road data
  //this is for each
  //double prevLat=-1;double prevLong=-1;
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
  //read car data
  file.open(car);
  string line; getline(file,line ); //this is for csv signature
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
    }
    else { //car does not exist in the dict - insert it
      st_car_point carpoint;
      carpoint.altitudeArray.push_back(stod(result[7]));
      carpoint.directionArray.push_back(result[9]);
      carpoint.directionArray.push_back(result[9]);
      carpoint.pointArray.push_back(coordinate_pair);
      carpoint.mobile_id = car_id;
      car_id_to_car_struct.insert(pair<long,st_car_point>(carpoint.mobile_id,carpoint));
    }
  }
  //for each car element find the closest point in the map using haversine
  //check direction and form more points
  for (auto& car_id : car_id_to_car_struct) { //each car id
    for (int i=0;i<car_id.second.pointArray.size();i++){ // each location of car
      coordinatetype carpointfromgps = car_id.second.pointArray[i];
      cout<<carpointfromgps.first<<" "<<carpointfromgps.second<<endl;
      vector<coordinatetype> generatedcarpoints(3);
      generatedcarpoints[1] = carpointfromgps;
      string direction = car_id.second.directionArray[i];
      GenerateDirectionalPoints(generatedcarpoints, direction);
      map<int,pair<coordinatetype,double>> distanceMap;
      vector<double> distance={500,500,500};
      for (int j=0;j<generatedcarpoints.size();j++) { //for each generated point
        //findgrid id
        gridtype carpointgrid_id = FindGridId(generatedcarpoints[j], minLattitude, minLongtitude);
        vector<gridtype> neighborGridPoints;
        neighborGridPoints.push_back(carpointgrid_id);
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first+1,carpointgrid_id.second));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first-1,carpointgrid_id.second));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first+1,carpointgrid_id.second+1));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first-1,carpointgrid_id.second+1));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first,carpointgrid_id.second+1));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first,carpointgrid_id.second-1));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first+1,carpointgrid_id.second-1));
        neighborGridPoints.push_back(make_pair(carpointgrid_id.first-1,carpointgrid_id.second-1));
        for (const auto& gridPoint : neighborGridPoints){ //for each grid from the neighbors
          const auto& gridpoint  = grid_to_coord.find(gridPoint);
          if (gridpoint == grid_to_coord.end())
            continue;//no elements in this grid,search in the neighbor point
          for (coordinatetype mappoint : gridpoint->second) //each map point in the grid
          {
            //cout<<"prev point size:"<<coord_to_loc_struct[mappoint].previous_point.size()<<endl;
            //for each prev point, we can calculate line distance as well but of course there won't be previous for
            //all points
            double dist = haversine(generatedcarpoints[j], mappoint); //we wanna do it for all the three points in a pipeline
            if (distance[j] > dist) {
              distance[j] = dist;
              distanceMap[j] = make_pair(mappoint,dist);
            }
          }
        }
      }
      if(distanceMap[0].first == distanceMap[1].first ) { //all the points are aligned
        car_id.second.route.push_back(distanceMap[0].first);
      }
      else if(distanceMap[0].first == distanceMap[2].first){
        car_id.second.route.push_back(distanceMap[0].first);
      }
      else if(distanceMap[1].first == distanceMap[2].first){
        car_id.second.route.push_back(distanceMap[1].first);
      }
      else {
        car_id.second.route.push_back(distanceMap[1].first);
      }
      unsigned long size = car_id.second.route.size();
      //print car route to file
      std::cout <<car_id.second.mobile_id <<","<<car_id.second.route[size-1].first<<","<<
      car_id.second.route[size-1].second<<","<<size<<'\n';
      //draw the routes
    }
  }
  return 0;
}
