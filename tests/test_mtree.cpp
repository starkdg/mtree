#include <iostream>
#include <iomanip>
#include <random>
#include <vector>
#include <cstdint>
#include <cassert>
#include "mtree/mtree.hpp"

using namespace std;
using namespace mt;

static long long m_id = 1;
static long long g_id = 10001;
static random_device rd;
static mt19937_64 gen(rd());
static uniform_int_distribution<uint64_t> distrib(0);

struct KeyObject {
	uint64_t key;
	KeyObject(){};
	KeyObject(const uint64_t key):key(key){}
	KeyObject(const KeyObject &other){
		key = other.key;
	}
	KeyObject& operator=(const KeyObject &other){
		key = other.key;
		return *this;
	}
	const double distance(const KeyObject &other)const{
		return __builtin_popcountll(key^other.key);
	}
};

int generate_data(vector<Entry<KeyObject>> &entries, const int N){

	for (int i=0;i < N;i++){
		Entry<KeyObject> entry { .id = m_id++, .key = KeyObject(distrib(gen)) };
		entries.push_back(entry);
	}

	return entries.size();
}

int generate_cluster(vector<Entry<KeyObject>> &entries, uint64_t center, int N, int max_radius){
	static uniform_int_distribution<int> radius_distr(1, max_radius);
	static uniform_int_distribution<int> bitindex_distr(0, 63);
		
	uint64_t mask = 0x01;

	entries.push_back({ .id = g_id++, .key = KeyObject(center) });
	for (int i=0;i < N-1;i++){
		uint64_t val = center;
		int dist = radius_distr(gen);
		for (int j=0;j < dist;j++){
			val ^= (mask << bitindex_distr(gen));
		}
		entries.push_back({ .id = g_id++, .key = KeyObject(val) });
	}
	return N;
}

int main(int argc, char **argv){

	const int nroutes = 2;
	const int leafcap = 10;
	
	MTree<KeyObject, nroutes, leafcap>  mtree;

	cout << "Add Points: " << endl;
	const int N = 100;
	vector<Entry<KeyObject>> entries;
	generate_data(entries, N);
	assert(entries.size() == N);

	RoutingObject<KeyObject>::n_build_ops = 0;
	for (auto e: entries){
		mtree.Insert(e);
	}
	cout << "no. distance ops: " << RoutingObject<KeyObject>::n_build_ops << endl;
	
	entries.clear();

	int sz = mtree.size();
	assert(sz == N);
	cout << "tree size: " << mtree.size() << endl;

	cout << "Add clusters" << endl;
	const int NClusters = 10;
	uint64_t centers[NClusters];

	const int ClusterSize = 5;
	const double MaxRadius = 5;
	for (int i=0;i < NClusters;i++){

		centers[i] = distrib(gen);

		vector<Entry<KeyObject>> cluster;
		generate_cluster(cluster, centers[i], ClusterSize, MaxRadius);
		assert(cluster.size() == ClusterSize);
		
		for (auto e : cluster){
			mtree.Insert(e);
		}

		sz = mtree.size();
		assert(sz == N + (i+1)*ClusterSize);
		cluster.clear();
	}

	sz = mtree.size();
	assert(sz == N + NClusters*ClusterSize);
	
	DBEntry<KeyObject>::n_query_ops = 0;
	
	for (int i=0;i < NClusters;i++){
		cout << "Query[" << dec << i << "] " << hex << centers[i] << endl;
		vector<Entry<KeyObject>> results = mtree.RangeQuery(KeyObject(KeyObject(centers[i])), MaxRadius);
		cout << "=>Found: " << dec << results.size() << " entries" << endl;
		assert(results.size() >= ClusterSize);
		results.clear();
	}

	cout << "avg. no. ops: " << (double)DBEntry<KeyObject>::n_query_ops/(double)NClusters/(double)sz  << endl;
	
	size_t nbytes = mtree.memory_usage();
	cout << "bytes in use: " << nbytes << " bytes" << endl;

	int ndels = mtree.DeleteEntry({0, KeyObject(centers[0]) });
	cout << "no. deleted: " << ndels << endl;

	sz = mtree.size();
	cout << "size: " << sz << endl;
	assert(sz == N + NClusters*ClusterSize - ndels);


	cout << "Query " << hex << centers[0] << endl;
	vector<Entry<KeyObject>> results = mtree.RangeQuery(centers[0], MaxRadius);
	cout << "=>Found: " << dec << results.size() << " entries" << endl;
	for (int i=0;i < (int)results.size();i++){
		cout << "    (" << dec << results[i].id << ") " << hex << results[i].key.key << endl;
	}
	assert((int)results.size() >= ClusterSize - ndels);	

	cout << "Clear All Entries" << endl;
	mtree.Clear();

	sz = mtree.size();
	assert(sz == 0);

	cout << "Done." << endl;
	
	return 0;
}
