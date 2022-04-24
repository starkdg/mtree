#ifndef _ENTRY_H
#define _ENTRY_H

#include <cstdint>

template<typename T>
struct  Entry {
	long long id;
	T key;
	Entry(){}
	Entry(const long long id, const T key):id(id),key(key){};
	Entry(const Entry &other){
		id = other.id;
		key = other.key;
	}
	const Entry& operator=(const Entry &other){
		id = other.id;
		key = other.key; 
		return *this;
	}
};

template<typename T>
struct RoutingObject {
	long long id;
	T key;
	void *subtree;
	double cover_radius;
	double d;
	RoutingObject():id(0),subtree(NULL){}
	RoutingObject(const long long id, const T key):id(id),key(key),subtree(0),cover_radius(0),d(0){}
	RoutingObject(const RoutingObject &other){
		id = other.id;
		key = other.key;
		subtree = other.subtree;
		cover_radius = other.cover_radius;
		d = other.d;
	}
	const RoutingObject& operator=(const RoutingObject &other){
		id = other.id;
		key = other.key;
		subtree = other.subtree;
		cover_radius = other.cover_radius;
		d = other.d;
		return *this;
	}
};

template<typename T>
struct DBEntry {
	long long id;
	T key;
	double d;
	DBEntry(const long long id, const T key, const double d):id(id),key(key),d(d){}
	DBEntry(const DBEntry &other){
		id = other.id;
		key = other.key;
		d = other.d;
	}
	const DBEntry& operator=(const DBEntry &other){
		id = other.id;
		key = other.key;
		d = other.d;
		return *this;
	}
	const double distance(const T &other)const{
		return key.distance(other.key);
	}
};

#endif /* _ENTRY_H */
