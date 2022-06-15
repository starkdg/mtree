/**
    MTree distance-based indexing structure
    Copyright (C) 2022  David G. Starkweather starkdg@gmx.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. 

**/

#ifndef _MTREE_H
#define _MTREE_H

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <map>
#include "mtree/mnode.hpp"
#include "mtree/entry.hpp"

namespace mt {

	template<typename T, int NROUTES=16, int LEAFCAP=250>
	class MTree {
	private:

		size_t m_count;
	
		MNode<T,NROUTES,LEAFCAP> *m_top;
		
		void promote(std::vector<DBEntry<T>> &entries, RoutingObject<T> &op1, RoutingObject<T> &op2);
	
		void partition(std::vector<DBEntry<T>> &entries, RoutingObject<T> &op1, RoutingObject<T> &op2,
					   std::vector<DBEntry<T>> &entries1, std::vector<DBEntry<T>> &entries2);

		MNode<T,NROUTES,LEAFCAP>* split(MNode<T,NROUTES,LEAFCAP> *node, const Entry<T> &nobj);
	
		void StoreEntries(MLeaf<T,NROUTES,LEAFCAP> *leaf, std::vector<DBEntry<T>> &entries);
	
	public:

		MTree():m_count(0),m_top(NULL){}

		
		void Insert(const Entry<T> &entry);

		const int DeleteEntry(const Entry<T> &entry);

		void Clear();

	
		const std::vector<Entry<T>> RangeQuery(T query, const double radius)const;

		const size_t size()const;

		//void PrintTree()const;
		
		const size_t memory_usage()const;
	};

}
	
/**
 *  MTree implementation code 
 *
 **/
template<typename T, int NROUTES, int LEAFCAP>
void mt::MTree<T,NROUTES,LEAFCAP>::promote(std::vector<DBEntry<T>> &entries,
										   RoutingObject<T> &robj1,
										   RoutingObject<T> &robj2){

	RoutingObject<T> routes[2];

	int current = 0;
	routes[current%2].key = entries[0].key;

	const int n_iters = 5;
	for (int i=0;i < n_iters;i++){
		int maxpos = -1;
		double maxd = 0;
		const int slimit = entries.size();
		for (int j=0;j < slimit;j++){
			double d = routes[current%2].distance(entries[j].key);
			if (d > maxd){
				maxpos = j;
				maxd = d;
			}
		}
		routes[++current%2].key = entries[maxpos].key;
	}
	
	robj1.key = routes[0].key;
	robj2.key = routes[1].key;
	robj1.d = 0;
	robj2.d = 0;

}

template<typename T, int NROUTES, int LEAFCAP>
void mt::MTree<T,NROUTES,LEAFCAP>::partition(std::vector<DBEntry<T>> &entries,
											 RoutingObject<T> &robj1,
											 RoutingObject<T> &robj2,
											 std::vector<DBEntry<T>> &entries1,
											 std::vector<DBEntry<T>> &entries2){

	double radius1 = 0;
	double radius2 = 0;
	for (int i=0;i < (int)entries.size();i++){
		double d1 = robj1.distance(entries[i].key); //distance(entries[i].key, robj1.key);
		double d2 = robj2.distance(entries[i].key); //distance(entries[i].key, robj2.key);
		if (d1 < d2){
			entries1.push_back({ entries[i].id, entries[i].key, d1 });
			if (d1 > radius1) radius1 = d1;
		} else {
			entries2.push_back({ entries[i].id, entries[i].key, d2 });
			if (d2 > radius2) radius2 = d2;
		}
	}
	
	robj1.cover_radius = radius1;
	robj2.cover_radius = radius2;
	entries.clear();
}

template<typename T, int NROUTES, int LEAFCAP>
void mt::MTree<T,NROUTES,LEAFCAP>::StoreEntries(MLeaf<T,NROUTES,LEAFCAP> *leaf,
												std::vector<DBEntry<T>> &entries){
	while (!entries.empty()){
		leaf->StoreEntry(entries.back());
		entries.pop_back();
	}
}

template<typename T, int NROUTES, int LEAFCAP>
mt::MNode<T,NROUTES,LEAFCAP>* mt::MTree<T,NROUTES,LEAFCAP>::split(MNode<T,NROUTES,LEAFCAP> *node, const Entry<T> &nobj){
	assert(typeid(*node) == typeid(MLeaf<T,NROUTES,LEAFCAP>));

	MLeaf<T,NROUTES,LEAFCAP> *leaf = (MLeaf<T,NROUTES,LEAFCAP>*)node;
	MLeaf<T,NROUTES,LEAFCAP> *leaf2 = new MLeaf<T,NROUTES,LEAFCAP>();

	std::vector<DBEntry<T>> entries;
	leaf->GetEntries(entries);

	entries.push_back({ .id = nobj.id, .key = nobj.key, 0 });

	RoutingObject<T> robj1, robj2;
	promote(entries, robj1, robj2);

	std::vector<DBEntry<T>> entries1, entries2;
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
void mt::MTree<T,NROUTES,LEAFCAP>::Insert(const Entry<T> &entry){

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
				if (!node->isfull()){
					((MLeaf<T,NROUTES,LEAFCAP>*)node)->StoreEntry({ entry.id, entry.key, d });
				} else {
					node = split(node, entry);
					if (node->isroot()){
						m_top = node;
					}
				}
				node = NULL;
			} else {
				throw std::logic_error("no such node type");
			}
		} while (node != NULL);
	}

	m_count += 1;
	
}

template<typename T, int NROUTES, int LEAFCAP>
const int mt::MTree<T,NROUTES,LEAFCAP>::DeleteEntry(const Entry<T> &entry){
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
			throw std::logic_error("no such node type");
		}
	};

	m_count -= count;
	return count;
}

template<typename T, int NROUTES, int LEAFCAP>
void mt::MTree<T,NROUTES,LEAFCAP>::Clear(){
	std::queue<MNode<T,NROUTES,LEAFCAP>*> nodes;
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
			throw std::logic_error("no such node type");
		}
		nodes.pop();
	}
	m_count = 0;
}

template<typename T, int NROUTES, int LEAFCAP>
const std::vector<mt::Entry<T>> mt::MTree<T,NROUTES,LEAFCAP>::RangeQuery(T query, const double radius)const{
	std::vector<Entry<T>> results;
	std::queue<MNode<T,NROUTES,LEAFCAP>*> nodes;

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
			throw std::logic_error("no such node type");
		}
		nodes.pop();
	}
	return results;
}

template<typename T, int NROUTES, int LEAFCAP>
const size_t mt::MTree<T,NROUTES,LEAFCAP>::size()const{
	return m_count;
}

template<typename T, int NROUTES, int LEAFCAP>
const size_t mt::MTree<T,NROUTES,LEAFCAP>::memory_usage()const{
	std::queue<MNode<T,NROUTES,LEAFCAP>*> nodes;
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
