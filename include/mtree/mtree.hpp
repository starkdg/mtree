#ifndef _MTREE_H
#define _MTREE_H

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <map>
#include "mtree/mnode.hpp"
#include "mtree/entry.hpp"

using namespace std;


template<typename T, int NROUTES=16, int LEAFCAP=250>
class MTree {
private:

	size_t m_count;
	
	MNode<T,NROUTES,LEAFCAP> *m_top;

	void promote(vector<DBEntry<T>> &entries, RoutingObject<T> &op1, RoutingObject<T> &op2);
	
	void partition(vector<DBEntry<T>> &entries, RoutingObject<T> &op1, RoutingObject<T> &op2,
				   vector<DBEntry<T>> &entries1, vector<DBEntry<T>> &entries2);

	MNode<T,NROUTES,LEAFCAP>* split(MNode<T,NROUTES,LEAFCAP> *node, const Entry<T> &nobj);
	
	void StoreEntries(MLeaf<T,NROUTES,LEAFCAP> *leaf, vector<DBEntry<T>> &entries);
	
public:

	MTree():m_count(0),m_top(NULL){}


	void Insert(const Entry<T> &entry);

	const int DeleteEntry(const Entry<T> &entry);

	void Clear();

	
	const vector<Entry<T>> RangeQuery(T query, const double radius)const;

	const size_t size()const;

	//void PrintTree()const;

	const size_t memory_usage()const;
};

/**
 *  MTree implementation code 
 *
 **/
template<typename T, int NROUTES, int LEAFCAP>
void MTree<T,NROUTES,LEAFCAP>::promote(vector<DBEntry<T>> &entries, RoutingObject<T> &robj1, RoutingObject<T> &robj2){
	int current = 0;
	T routes[2];
	routes[current%2] = entries[0].key;

	const int n_iters = 5;
	for (int i=0;i < n_iters;i++){
		int maxpos = -1;
		double maxd = 0;
		for (int j=0;j < (int)entries.size();j++){
			double d = entries[j].distance(routes[current%2]); 
			if (d > maxd){
				maxpos = j;
				maxd = d;
			}
		}

		routes[(++current)%2] = entries[maxpos].key;
	}
	
	robj1.key = routes[0];
	robj2.key = routes[1];
	robj1.d = 0;
	robj2.d = 0;
}

template<typename T, int NROUTES, int LEAFCAP>
void MTree<T,NROUTES,LEAFCAP>::partition(vector<DBEntry<T>> &entries, RoutingObject<T> &robj1, RoutingObject<T> &robj2,
					  vector<DBEntry<T>> &entries1, vector<DBEntry<T>> &entries2){

	double radius1 = 0;
	double radius2 = 0;
	for (int i=0;i < (int)entries.size();i++){
		double d1 = entries[i].distance(robj1.key); //distance(entries[i].key, robj1.key);
		double d2 = entries[i].distance(robj2.key); //distance(entries[i].key, robj2.key);
		if (d1 <= d2){
			entries[i].d = d1;
			entries1.push_back(entries[i]);
			if (d1 > radius1) radius1 = d1;
		} else {
			entries[i].d = d2;
			entries2.push_back(entries[i]);
			if (d2 > radius2) radius2 = d2;
		}
	}
	
	robj1.cover_radius = radius1;
	robj2.cover_radius = radius2;
}

template<typename T, int NROUTES, int LEAFCAP>
void MTree<T,NROUTES,LEAFCAP>::StoreEntries(MLeaf<T,NROUTES,LEAFCAP> *leaf, vector<DBEntry<T>> &entries){
	while (!entries.empty()){
		leaf->StoreEntry(entries.back());
		entries.pop_back();
	}
}

template<typename T, int NROUTES, int LEAFCAP>
MNode<T,NROUTES,LEAFCAP>* MTree<T,NROUTES,LEAFCAP>::split(MNode<T,NROUTES,LEAFCAP> *node, const Entry<T> &nobj){
	assert(typeid(*node) == typeid(MLeaf<T,NROUTES,LEAFCAP>));

	MLeaf<T,NROUTES,LEAFCAP> *leaf = (MLeaf<T,NROUTES,LEAFCAP>*)node;
	MLeaf<T,NROUTES,LEAFCAP> *leaf2 = new MLeaf<T,NROUTES,LEAFCAP>();

	vector<DBEntry<T>> entries;
	leaf->GetEntries(entries);
	entries.push_back({ .id = nobj.id, .key = nobj.key, 0 });

	RoutingObject<T> robj1, robj2;
	promote(entries, robj1, robj2);

	vector<DBEntry<T>> entries1, entries2;
	partition(entries, robj1, robj2, entries1, entries2);
	robj1.subtree = leaf;
	robj2.subtree = leaf2;

	leaf->Clear();
	
	StoreEntries(leaf, entries1);
	StoreEntries(leaf2, entries2);

	MInternal<T,NROUTES,LEAFCAP> *pnode;
	if (node->isroot()){ // root level
		MInternal<T,NROUTES,LEAFCAP> *qnode = new MInternal<T,NROUTES,LEAFCAP>();

		int rdx = qnode->StoreRoute(robj1);
		qnode->SetChildNode(leaf, rdx);

		rdx = qnode->StoreRoute(robj2);
		qnode->SetChildNode(leaf2, rdx);

		pnode = qnode;
	} else {  // not root  
		int rdx;
		pnode = (MInternal<T,NROUTES,LEAFCAP>*)(node->GetParentNode(rdx));
		if (pnode->isfull()){ // parent node overflows
			MInternal<T,NROUTES,LEAFCAP> *qnode = new MInternal<T,NROUTES,LEAFCAP>();
			
			RoutingObject<T> pobj;
			pnode->GetRoute(rdx, pobj);

			robj1.d = pobj.distance(robj1.key); //  distance(robj1.key, pobj.key);
			
			int rdx1 = qnode->StoreRoute(robj1);
			qnode->SetChildNode(leaf, rdx1);

			robj2.d = pobj.distance(robj2.key); // distance(robj2.key, pobj.key);
			
			int rdx2 = qnode->StoreRoute(robj2);
			qnode->SetChildNode(leaf2, rdx2);

			pnode->SetChildNode(qnode, rdx);

		} else { // still room in parent node
			int gdx;
			MInternal<T,NROUTES,LEAFCAP> *gnode = (MInternal<T,NROUTES,LEAFCAP>*)pnode->GetParentNode(gdx);
			if (gnode != NULL){
				RoutingObject<T> pobj;
				gnode->GetRoute(gdx, pobj);
				robj1.d = pobj.distance(robj1.key); // distance(robj1.key, pobj.key);
				robj2.d = pobj.distance(robj2.key); // distance(robj2.key, pobj.key);
			}
			
			pnode->ConfirmRoute(robj1, rdx);
			pnode->SetChildNode(leaf, rdx);

			int rdx2 = pnode->StoreRoute(robj2);
			pnode->SetChildNode(leaf2, rdx2);
		}
	}

	return pnode;
}

template<typename T, int NROUTES, int LEAFCAP>
void MTree<T,NROUTES,LEAFCAP>::Insert(const Entry<T> &entry){

	MNode<T,NROUTES,LEAFCAP> *node = m_top;
	if (node == NULL){ // add first entry to empty tree
	    MLeaf<T,NROUTES,LEAFCAP> *leaf = new MLeaf<T,NROUTES,LEAFCAP>();
		DBEntry<T> dentry(entry.id, entry.key, 0);
		leaf->StoreEntry(dentry);
		m_top = leaf;
	} else {
		double d = 0;
		do {
			if (typeid(*node) == typeid(MInternal<T,NROUTES,LEAFCAP>)){
				RoutingObject<T>  robj;
				((MInternal<T,NROUTES,LEAFCAP>*)node)->SelectRoute(entry.key, robj, true);
				node = (MNode<T,NROUTES,LEAFCAP>*)robj.subtree;
				d = robj.key.distance(entry.key);  // distance(robj.key, entry.key);
			} else if (typeid(*node) == typeid(MLeaf<T,NROUTES,LEAFCAP>)){
				MLeaf<T,NROUTES,LEAFCAP> *leaf = (MLeaf<T,NROUTES,LEAFCAP>*)node;
				if (!node->isfull()){
					DBEntry<T> dentry(entry.id, entry.key, d);
					leaf->StoreEntry(dentry);
				} else {
					node = split(node, entry);
					if (node->isroot()){
						m_top = node;
					}
				}
				node = NULL;
			} else {
				throw logic_error("no such node type");
			}
		} while (node != NULL);
	}

	m_count += 1;
	
}

template<typename T, int NROUTES, int LEAFCAP>
const int MTree<T,NROUTES,LEAFCAP>::DeleteEntry(const Entry<T> &entry){
	MNode<T,NROUTES,LEAFCAP> *node = m_top;

	int count = 0;
	while (node != NULL){
		if (typeid(*node) == typeid(MInternal<T,NROUTES,LEAFCAP>)){
			RoutingObject<T> robj;
			((MInternal<T,NROUTES,LEAFCAP>*)node)->SelectRoute(entry.key, robj, false);
			node = (MNode<T,NROUTES,LEAFCAP>*)robj.subtree;
		} else if (typeid(*node) == typeid(MLeaf<T,NROUTES,LEAFCAP>)){
			MLeaf<T,NROUTES,LEAFCAP> *leaf = (MLeaf<T,NROUTES,LEAFCAP>*)node;
			count = leaf->DeleteEntry(entry.key);
			node = NULL;
		} else {
			throw logic_error("no such node type");
		}
	};

	m_count -= count;
	return count;
}

template<typename T, int NROUTES, int LEAFCAP>
void MTree<T,NROUTES,LEAFCAP>::Clear(){
	queue<MNode<T,NROUTES,LEAFCAP>*> nodes;
	if (m_top != NULL)
		nodes.push(m_top);

	while (!nodes.empty()){
		MNode<T,NROUTES,LEAFCAP> *current = nodes.front();
		if (typeid(*current) == typeid(MInternal<T,NROUTES,LEAFCAP>)){
			MInternal<T,NROUTES,LEAFCAP> *internal = (MInternal<T,NROUTES,LEAFCAP>*)current;
			for (int i=0;i < NROUTES;i++){
				MNode<T,NROUTES,LEAFCAP> *child = current->GetChildNode(i);
				if (child) nodes.push(child);
			}
			delete internal;		
		} else if (typeid(*current) == typeid(MLeaf<T,NROUTES,LEAFCAP>)){
			MLeaf<T,NROUTES,LEAFCAP> *leaf = (MLeaf<T,NROUTES,LEAFCAP>*)current;
			delete leaf;
		} else {
			throw logic_error("no such node type");
		}
		nodes.pop();
	}
	m_count = 0;
}

template<typename T, int NROUTES, int LEAFCAP>
const vector<Entry<T>> MTree<T,NROUTES,LEAFCAP>::RangeQuery(T query, const double radius)const{
	vector<Entry<T>> results;
	queue<MNode<T,NROUTES,LEAFCAP>*> nodes;

	if (m_top != NULL)
		nodes.push(m_top);

	while (!nodes.empty()){
		MNode<T,NROUTES,LEAFCAP> *current = nodes.front();
		if (typeid(*current) == typeid(MInternal<T,NROUTES,LEAFCAP>)){
			MInternal<T,NROUTES,LEAFCAP> *internal = (MInternal<T,NROUTES,LEAFCAP>*)current;
			internal->SelectRoutes(query, radius, nodes);
		} else if (typeid(*current) == typeid(MLeaf<T,NROUTES,LEAFCAP>)){
			MLeaf<T,NROUTES,LEAFCAP> *leaf = (MLeaf<T,NROUTES,LEAFCAP>*)current;
			leaf->SelectEntries(query, radius, results);
		} else {
			throw logic_error("no such node type");
		}
		nodes.pop();
	}
	return results;
}

template<typename T, int NROUTES, int LEAFCAP>
const size_t MTree<T,NROUTES,LEAFCAP>::size()const{
	return m_count;
}

/**
template<typename T, int NROUTES, int LEAFCAP>
void MTree<T,NROUTES,LEAFCAP>::PrintTree()const{
	map<int, MNode<T,NROUTES,LEAFCAP>*> nodes, childnodes;

	if (m_top != NULL)
		nodes[0] = m_top;

	int level = 0;
	while (!nodes.empty()){
		cout << "(Level = " << dec << level << ")" << endl;
		

		for (auto iter = nodes.begin(); iter != nodes.end(); iter++){
			MNode<T,NROUTES,LEAFCAP> *node = iter->second;
			
			for (int i=0;i < NROUTES;i++){
				MNode<T,NROUTES,LEAFCAP> *child = node->GetChildNode(i);
				if (child) childnodes[iter->first*NROUTES + i] = child;

			}

			if (typeid(*node) == typeid(MInternal<T,NROUTES,LEAFCAP>)){
				cout << "Internal[" <<  dec << iter->first << "]" << endl;
				
			} else if (typeid(*node) == typeid(MLeaf<T,NROUTES,LEAFCAP>)){
				MLeaf<T,NROUTES,LEAFCAP> *leaf = (MLeaf<T,NROUTES,LEAFCAP>*)node;

				cout << "  Leaf[" << dec << iter->first << "] size = " << leaf->size() << endl;

				vector<DBEntry<T>> entries;
				leaf->GetEntries(entries);
				for (DBEntry<T> e : entries){
					cout << "    " << dec << e.id << " " << hex << e.key.key << endl;
				}
				
			} else {
				cout << "no such node type" << endl;
			}
		}
		nodes.clear();
		nodes = move(childnodes);
		level += 1;
	}
}
**/

template<typename T, int NROUTES, int LEAFCAP>
const size_t MTree<T,NROUTES,LEAFCAP>::memory_usage()const{
	queue<MNode<T,NROUTES,LEAFCAP>*> nodes;
	if (m_top != NULL)
		nodes.push(m_top);

	int n_internal = 0, n_leaf = 0, n_entry = 0;
	while (!nodes.empty()){
		MNode<T,NROUTES,LEAFCAP> *node = nodes.front();
		if (typeid(*node) == typeid(MInternal<T,NROUTES,LEAFCAP>)){
				n_internal++;
				for (int i=0;i < NROUTES;i++){
					MNode<T,NROUTES,LEAFCAP> *child = node->GetChildNode(i);
					if (child) nodes.push(child);
				}
		} else if (typeid(*node) == typeid(MLeaf<T,NROUTES,LEAFCAP>)){
			n_leaf++;
			n_entry += node->size();
		}
		nodes.pop();
	}

	return (n_internal*sizeof(MInternal<T,NROUTES,LEAFCAP>) + n_leaf*sizeof(MLeaf<T,NROUTES,LEAFCAP>)
			+ m_count*sizeof(DBEntry<T>) + sizeof(MTree<T,NROUTES,LEAFCAP>));
}

#endif
