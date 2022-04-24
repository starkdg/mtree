#ifndef _MNODE_H
#define _MNODE_H

#include <vector>
#include <array>
#include <queue>
#include <cfloat>
#include "mtree/entry.hpp"

using namespace std;

/**
 * template parameters: NROUTES - no. routes to store in internal nodes
 *                      LEAFCAP - no. dbentries to store in leaf nodes
 **/

template<typename T, int NROUTES=16, int LEAFCAP=200>
class MNode {
protected:

	MNode<T,NROUTES,LEAFCAP> *p;   // parent node
	int rindex; // route_entry index to parent node from this node
	
public:

	MNode():p(NULL){};
	virtual ~MNode(){}
	
	virtual const int size()const = 0;

	virtual const bool isfull()const = 0;

	const bool isroot()const;

	MNode<T,NROUTES,LEAFCAP>* GetParentNode(int &rdx)const;

	void SetParentNode(MNode<T,NROUTES,LEAFCAP> *pnode, const int rdx);

	virtual void SetChildNode(MNode<T,NROUTES,LEAFCAP>* child, const int rdx) = 0;

	virtual MNode<T,NROUTES,LEAFCAP>* GetChildNode(const int rdx)const = 0;

	virtual void Clear() = 0;

};


template<typename T, int NROUTES, int LEAFCAP>
class MInternal : public MNode<T,NROUTES,LEAFCAP> {
protected:

	int n_routes;
	RoutingObject<T> routes[NROUTES];
	
public:
	MInternal();
	~MInternal(){}
	
	const int size()const;

	const bool isfull()const;

	// return all the routing objects for this node 
	void GetRoutes(vector<RoutingObject<T>> &routes)const;

	// select routing object to follow for insert
	// modify cover radius in routing object as appropriate
	int SelectRoute(const T nobj, RoutingObject<T> &robj, bool insert);

	void SelectRoutes(const T query, const double radius, queue<MNode<T,NROUTES,LEAFCAP>*> &nodes)const;
	
	int StoreRoute(const RoutingObject<T> &robj);

	void ConfirmRoute(const RoutingObject<T> &robj, const int rdx);

	void GetRoute(const int rdx, RoutingObject<T> &route);

	void SetChildNode(MNode<T,NROUTES,LEAFCAP> *child, const int rdx);

	MNode<T,NROUTES,LEAFCAP>* GetChildNode(const int rdx)const;
	
	void Clear();
};

template<typename T, int NROUTES, int LEAFCAP>
class MLeaf : public MNode<T,NROUTES,LEAFCAP> {
protected:

	vector<DBEntry<T>> entries;

public:
	MLeaf(){};
	~MLeaf(){}
	
	const int size()const;
	const bool isfull()const;

	int StoreEntry(DBEntry<T> &nobj);

	void GetEntries(vector<DBEntry<T>> &dbentries)const;

	void SelectEntries(const T query, const double radius, vector<Entry<T>> &results)const;

	int DeleteEntry(const T &entry);
	
	void SetChildNode(MNode<T,NROUTES,LEAFCAP> *child, const int rdx);

	MNode<T,NROUTES,LEAFCAP>* GetChildNode(const int rdx)const;

	void Clear();
};


/**
 *  MNode base class implementation 
 *
 **/

template<typename T, int NROUTES, int LEAFCAP>
const bool MNode<T,NROUTES,LEAFCAP>::isroot()const{
	return (p == NULL);
}

template<typename T, int NROUTES, int LEAFCAP>
MNode<T,NROUTES,LEAFCAP>* MNode<T,NROUTES,LEAFCAP>::GetParentNode(int &rdx)const{
	rdx = rindex;
	return p;
}

template<typename T, int NROUTES, int LEAFCAP>
void MNode<T,NROUTES,LEAFCAP>::SetParentNode(MNode<T,NROUTES,LEAFCAP> *pnode, const int rdx){
	assert(rdx >= 0 && rdx < NROUTES);
	p = pnode;
	rindex = rdx;
}

/**
 *
 *  MInternal Node implementation
 *
 **/

template<typename T, int NROUTES, int LEAFCAP>
MInternal<T,NROUTES,LEAFCAP>::MInternal(){
	n_routes = 0;
	for (int i=0;i < NROUTES;i++){
		routes[i].subtree = NULL;
	}
}

template<typename T, int NROUTES, int LEAFCAP>
const int MInternal<T,NROUTES,LEAFCAP>::size()const{
	return n_routes;
}

template<typename T, int NROUTES, int LEAFCAP>
const bool MInternal<T,NROUTES,LEAFCAP>::isfull()const{
	return (n_routes >= NROUTES);
}


template<typename T, int NROUTES, int LEAFCAP>
void MInternal<T,NROUTES,LEAFCAP>::GetRoutes(vector<RoutingObject<T>> &routes)const{
	for (int i=0;i < NROUTES;i++){
		if (routes[i].subtree != NULL)
			routes.push_back(routes[i]);
	}
}

template<typename T, int NROUTES, int LEAFCAP>
int MInternal<T,NROUTES,LEAFCAP>::SelectRoute(const T nobj, RoutingObject<T> &robj, bool insert){

	int min_pos = -1;
	double min_dist = DBL_MAX;
	
	for (int i=0;i < NROUTES;i++){
		if (routes[i].subtree != NULL){
			const double d = nobj.distance(routes[i].key); //distance(routes[i].key, nobj.key);
			if (d < min_dist){
				min_pos = i;
				min_dist = d;
			}
		}
	}

	if (min_pos < 0)
		throw logic_error("unable to find route entry");

	if (insert && min_dist > routes[min_pos].cover_radius)
		routes[min_pos].cover_radius = min_dist;
	
	robj = routes[min_pos];
	
	return min_pos;
}

template<typename T, int NROUTES, int LEAFCAP>
void MInternal<T,NROUTES,LEAFCAP>::SelectRoutes(const T query, const double radius, queue<MNode<T,NROUTES,LEAFCAP>*> &nodes)const{
	RoutingObject<T> pobj;
	if (this->p != NULL)
		((MInternal<T,NROUTES,LEAFCAP>*)this->p)->GetRoute(this->rindex, pobj);

	double d = (this->p != NULL) ? query.distance(pobj.key) : 0;      // distance(pobj.key, query) : 0;
	for (int i=0;i < NROUTES;i++){
		if (routes[i].subtree != NULL){
			if (abs(d -  routes[i].d) <= radius + routes[i].cover_radius){
				if (query.distance(routes[i].key) <= radius + routes[i].cover_radius){   //distance(routes[i].key, query)
					nodes.push((MNode<T,NROUTES,LEAFCAP>*)routes[i].subtree);
				}
			}
		}
	}
	
}

template<typename T, int NROUTES, int LEAFCAP>
int MInternal<T,NROUTES,LEAFCAP>::StoreRoute(const RoutingObject<T> &robj){
	assert(n_routes < NROUTES);

	int index = -1;
	for (int i=0;i < NROUTES;i++){
		if (routes[i].subtree == NULL){
			routes[i] = robj;
			index = i;
			n_routes++;
			break;
		}
	}
	return index;
}

template<typename T, int NROUTES, int LEAFCAP>
void MInternal<T,NROUTES,LEAFCAP>::ConfirmRoute(const RoutingObject<T> &robj, const int rdx){
	assert(rdx >= 0 && rdx < NROUTES && robj.subtree != NULL);
	routes[rdx] = robj;
}

template<typename T, int NROUTES, int LEAFCAP>
void MInternal<T,NROUTES,LEAFCAP>::GetRoute(const int rdx, RoutingObject<T> &route){
	assert(rdx >= 0 && rdx < NROUTES);
	route = routes[rdx];
};

template<typename T, int NROUTES, int LEAFCAP>
void MInternal<T,NROUTES,LEAFCAP>::SetChildNode(MNode<T,NROUTES,LEAFCAP> *child, const int rdx){
	assert(rdx >= 0 && rdx < NROUTES);
	routes[rdx].subtree = child;
	child->SetParentNode(this, rdx);
}

template<typename T, int NROUTES, int LEAFCAP>
MNode<T,NROUTES,LEAFCAP>* MInternal<T,NROUTES,LEAFCAP>::GetChildNode(const int rdx)const{
	assert(rdx >= 0 && rdx < NROUTES);
	return (MNode<T,NROUTES,LEAFCAP>*)routes[rdx].subtree;
}

template<typename T, int NROUTES, int LEAFCAP>
void MInternal<T,NROUTES,LEAFCAP>::Clear(){
	return;
}

/**
 *
 * MLeaf<T,NROUTES,LEAFCAP> node implementation
 *
 **/

template<typename T, int NROUTES, int LEAFCAP>
const int MLeaf<T,NROUTES,LEAFCAP>::size()const{
	return entries.size();
}

template<typename T, int NROUTES, int LEAFCAP>
const bool MLeaf<T,NROUTES,LEAFCAP>::isfull()const{
	return (entries.size() >= LEAFCAP);
}

template<typename T, int NROUTES, int LEAFCAP>
int MLeaf<T,NROUTES,LEAFCAP>::StoreEntry(DBEntry<T> &nobj){
	if (entries.size() >= LEAFCAP)
		throw out_of_range("full leaf node");

	int index = entries.size();
	entries.push_back(nobj);
	return index;
}

template<typename T, int NROUTES, int LEAFCAP>
void MLeaf<T,NROUTES,LEAFCAP>::GetEntries(vector<DBEntry<T>> &dbentries)const{
	for (auto &e : entries){
		dbentries.push_back(e);
	}
}

template<typename T, int NROUTES, int LEAFCAP>
void MLeaf<T,NROUTES,LEAFCAP>::SelectEntries(const T query, const double radius, vector<Entry<T>> &results)const{
	double d = 0;
	if (this->p != NULL){
		RoutingObject<T> pobj;
		((MInternal<T,NROUTES,LEAFCAP>*)this->p)->GetRoute(this->rindex, pobj);
		d = query.distance(pobj.key);               //distance(pobj.key, query);
	}
	for (int j=0;j < (int)entries.size();j++){
		if (abs(d -  entries[j].d) <= radius){
			if (entries[j].distance(query) <= radius){	//distance(entries[j].key, query) <= radius){
				results.push_back({ entries[j].id, entries[j].key });
			}
		}
	}
}

template<typename T, int NROUTES, int LEAFCAP>
int MLeaf<T,NROUTES,LEAFCAP>::DeleteEntry(const T &entry){
	int count = 0;

	double d = 0;
	if (this->p != NULL){
		RoutingObject<T> pobj;
		((MInternal<T,NROUTES,LEAFCAP>*)this->p)->GetRoute(this->rindex, pobj);
		d = pobj.key.distance(entry.key);            //distance(pobj.key, entry.key);
	}

	for (int j=0;j < (int)entries.size();j++){
		if (d == entries[j].d){
			if (entry.distance(entries[j].key) == 0){      //distance(entries[j].key, entry.key) == 0){
				entries[j] = entries.back();
				entries.pop_back();
				count++;
			}
		}
	}
	
	return count;
}

template<typename T, int NROUTES, int LEAFCAP>
void MLeaf<T,NROUTES,LEAFCAP>::SetChildNode(MNode<T,NROUTES,LEAFCAP> *child, const int rdx){
	return;
}

template<typename T, int NROUTES, int LEAFCAP>
MNode<T,NROUTES,LEAFCAP>* MLeaf<T,NROUTES,LEAFCAP>::GetChildNode(const int rdx)const{
	return NULL;
}

template<typename T, int NROUTES, int LEAFCAP>
void MLeaf<T,NROUTES,LEAFCAP>::Clear(){
	entries.clear();
}


#endif /* _MNODE_H */
