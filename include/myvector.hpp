#ifndef MYVECTOR_H
#define MYVECTOR_H

#include <vector>
#include <string>
#include <iostream>

typedef double coord; //myvector coordinates

class myvector {
    std::vector<coord> values;
    std::string id;
  public:
    myvector();
    myvector(const myvector& obj);
    myvector(std::vector<coord> &v, std::string &id);
    myvector(std::vector<coord> &v);  //empty id
    ~myvector();
    void print(std::ostream &out) const;
    int size() const;
    std::string get_id() const;
    std::vector<coord> getCoords() const;
    std::vector<coord>::const_iterator begin() const;
    std::vector<coord>::const_iterator end() const;
    coord& operator[](int i) {return values[i];};
};

typedef std::vector<myvector> MyVectorContainer;

#endif
