#ifndef _ENTRY_H
#define _ENTRY_H
#include <cstdint>

namespace mt {

	template<typename T>
	struct  Entry {
		long long id;
		T key;
		Entry(){}
		Entry(const long long id, const T key):id(id),key(key){};
		Entry(const Entry<T> &other){
			id = other.id;
			key = other.key;
		}
		const Entry<T>& operator=(const Entry<T> &other){
			id = other.id;
			key = other.key; 
			return *this;
		}
	};

	template<typename T>
	struct RoutingObject {
		static unsigned long n_build_ops;
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
		const double distance(const T &other)const{
			RoutingObject<T>::n_build_ops++;
			return key.distance(other);
		}
	};

	template<typename T>
	struct DBEntry {
		static unsigned long n_query_ops;
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
			DBEntry<T>::n_query_ops++;
			return key.distance(other);
		}
	};
}

template<typename T>
unsigned long mt::RoutingObject<T>::n_build_ops = 0;

template<typename T>
unsigned long mt::DBEntry<T>::n_query_ops = 0;

#endif /* _ENTRY_H */
