#include <iostream>
#include <iomanip>
#include <random>
#include <vector>
#include <cassert>
#include <cstring>
#include "mtree/mtree.hpp"

using namespace std;

static long long m_id = 1;
static long long g_id = 10001;
static random_device m_rd;
static mt19937_64 m_gen(m_rd());
static uniform_real_distribution<double> m_distrib(-1.0, 1.0);
static uniform_real_distribution<double> m_eps(-0.03, 0.03);

double Radius = 0.10;

#define KEYLEN 10

struct KeyObject {
	double key[KEYLEN];
	KeyObject(){};
	KeyObject(const double key[]){
		memcpy(this->key, key, KEYLEN*sizeof(double));
	}
	KeyObject(const KeyObject &other){
		memcpy(key, other.key, KEYLEN*sizeof(double));
	}
	KeyObject& operator=(const KeyObject &other){
		memcpy(key, other.key, KEYLEN*sizeof(double));
		return *this;
	}
	const double distance(const KeyObject &other)const{
		double d = 0;
		for (int i=0;i < KEYLEN;i++){
			d += pow(key[i] - other.key[i], 2.0);
		}
		return sqrt(d);
	}
};


int generate_center(double center[]){
	for (int i=0;i < KEYLEN;i++){
		center[i] = m_distrib(m_gen);
	}
	return 1;
}

int generate_data(vector<Entry<KeyObject>> &entries, const int N){
	double buf[KEYLEN];
	for (int i=0;i < N;i++){
		generate_center(buf);
		Entry<KeyObject> entry = { .id = m_id++, .key = KeyObject(buf) };
		entries.push_back(entry);
	}
	return entries.size();
}


int generate_cluster(vector<Entry<KeyObject>> &entries, double center[], int N){

	entries.push_back({ .id = g_id++, .key = KeyObject(center) });
	for (int i=0;i < N-1;i++){

		double v[KEYLEN];
		for (int j=0;j < KEYLEN;j++){
			v[j] = center[j] + m_eps(m_gen);
		}
	
		entries.push_back({ .id = g_id++, .key = KeyObject(v) });
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

	DBEntry<KeyObject>::n_ops = 0;
	for (auto e: entries){
		mtree.Insert(e);
	}
	cout << "no. distance ops: " << DBEntry<KeyObject>::n_ops << endl;
	
	entries.clear();

	int sz = mtree.size();
	assert(sz == N);
	cout << "tree size: " << mtree.size() << endl;

	cout << "Add clusters" << endl;
	const int NClusters = 10;
	double centers[NClusters][KEYLEN];

	const int ClusterSize = 5;
	const double MaxRadius = 5;
	for (int i=0;i < NClusters;i++){

		generate_center(centers[i]);

		vector<Entry<KeyObject>> cluster;
		generate_cluster(cluster, centers[i], ClusterSize);
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
	
	DBEntry<KeyObject>::n_ops = 0;
	
	for (int i=0;i < NClusters;i++){
		cout << "Query[" << dec << i << "] " << endl;
		vector<Entry<KeyObject>> results = mtree.RangeQuery(KeyObject(KeyObject(centers[i])), Radius);
		cout << "=>Found: " << dec << results.size() << " entries" << endl;
		assert(results.size() >= ClusterSize);
		results.clear();
	}

	cout << "avg. no. ops: " << (double)DBEntry<KeyObject>::n_ops/(double)NClusters << endl;
	
	size_t nbytes = mtree.memory_usage();
	cout << "bytes in use: " << nbytes << " bytes" << endl;

	int ndels = mtree.DeleteEntry({0, KeyObject(centers[0]) });
	cout << "no. deleted: " << ndels << endl;

	sz = mtree.size();
	cout << "size: " << sz << endl;
	assert(sz == N + NClusters*ClusterSize - ndels);


	cout << "Query " << endl;
	vector<Entry<KeyObject>> results = mtree.RangeQuery(centers[0], Radius);
	cout << "=>Found: " << dec << results.size() << " entries" << endl;
	for (int i=0;i < (int)results.size();i++){
		cout << "    (" << dec << results[i].id << ") " << endl;
	}
	assert((int)results.size() >= ClusterSize - ndels);	

	cout << "Clear All Entries" << endl;
	mtree.Clear();

	sz = mtree.size();
	assert(sz == 0);

	cout << "Done." << endl;
	
	return 0;
}
