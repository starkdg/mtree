#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <random>
#include <vector>
#include <chrono>
#include <cassert>
#include <ratio>
#include <cmath>
#include <cstring>
#include "mtree/mtree.hpp"

#define KEYLEN 16

using namespace std;
using namespace mt;

static long long m_id = 1;       // general id's
static long long g_id = 1000000; // cluster id's

const int n_runs = 5; // avg over n_runs


const int NR = 4;
const int LC = 50;

static random_device m_rd;
static mt19937_64 m_gen(m_rd());
static uniform_real_distribution<double> m_distrib(-1.0, 1.0);


struct KeyObject {
	double key[KEYLEN];
	KeyObject(){
		for (int i=0;i < KEYLEN;i++) this->key[i] = 0;
	};
	KeyObject(const double key[]){
		memcpy(this->key, key, KEYLEN*sizeof(double));
	}
	KeyObject(const KeyObject &other){
		memcpy(this->key, other.key, KEYLEN*sizeof(double));
	}
	KeyObject& operator=(const KeyObject &other){
		memcpy(this->key, other.key, KEYLEN*sizeof(double));
		return *this;
	}
	const double distance(const KeyObject &other)const{
		double sum = 0;
		for (int i=0;i < KEYLEN;i++){
			sum += pow(key[i] - other.key[i], 2.0);
		}
		return sqrt(sum);
	}
};

struct perfmetric {
	double avg_build_ops;
	double avg_build_time;
	double avg_query_ops;
	double avg_query_time;
	size_t avg_memory_used;
};


int generate_center(double center[]){
	for (int i=0;i < KEYLEN;i++){
		center[i] = m_distrib(m_gen);
	}
	return 1;
}

int generate_data(vector<Entry<KeyObject>> &entries, const int n){

	double buf[KEYLEN];
	for (int i=0;i < n;i++){
		generate_center(buf);
		Entry<KeyObject> entry = { .id = m_id++, .key = KeyObject(buf) };
		entries.push_back(entry);
	}
	return entries.size();
}


int generate_cluster(vector<Entry<KeyObject>> &entries, double center[], const int n, const double radius){
	double diff = sqrt(pow(radius, 2.0)/KEYLEN);
	uniform_real_distribution<double> m_eps(-diff, diff);

	double val[KEYLEN];
	entries.push_back({ .id = g_id++, .key = KeyObject(center) });
	for (int i=0;i < n-1;i++){
		memcpy(val, center, KEYLEN*sizeof(double));
		if (radius > 0){
			for (int j=0;j < KEYLEN;j++){
				val[j] = center[j] + m_eps(m_gen);
			}
		}
		entries.push_back({ .id = g_id++, .key = KeyObject(val) });
	}
	return n;
}

void do_run(const int index, const int n_entries, const int n_clusters,
			const int clustersize, const double radius, vector<struct perfmetric> &metrics){
	m_id = 1;
	g_id = 100000000;

	MTree<KeyObject,NR,LC>  mtree;

	size_t sz = 0;
	chrono::duration<double, nano> total(0);
	RoutingObject<KeyObject>::n_build_ops = 0;

	vector<Entry<KeyObject>> entries;
	generate_data(entries, n_entries);
	assert((int)entries.size() == n_entries);

	int count = 0;
	auto s = chrono::steady_clock::now();
	for (auto &e : entries){
		mtree.Insert(e);
	}
	auto e = chrono::steady_clock::now();
	total += (e - s);
	sz = mtree.size();
	assert(sz == n_entries*(i+1));

	struct perfmetric m;
	
	m.avg_build_ops = 100.0*((double)RoutingObject<KeyObject>::n_build_ops/(double)sz); 
	m.avg_build_time = total.count()/(double)sz;
	
	cout << "(" << index << ") build tree: " << setw(10) << setprecision(6) << m.avg_build_ops << "% opers "
		 << setw(10) << setprecision(6) << m.avg_build_time << " nanosecs ";


	double centers[n_clusters][KEYLEN];
	for (int i=0;i < n_clusters;i++){
		generate_center(centers[i]);
		vector<Entry<KeyObject>> cluster;
		generate_cluster(cluster, centers[i], clustersize, radius);
		assert(cluster.size() == clustersize);
		for (auto &e : cluster){
			mtree.Insert(e);
		}
		
		sz = mtree.size();
		assert(sz == n_entries + clustersize*(i+1));
	}

	DBEntry<KeyObject>::n_query_ops = 0;
	chrono::duration<double, milli> querytime(0);
	for (int i=0;i < n_clusters;i++){
		auto s = chrono::steady_clock::now();
		vector<Entry<KeyObject>> results = mtree.RangeQuery(KeyObject(centers[i]), radius);
		auto e = chrono::steady_clock::now();
		querytime += (e - s);

		int nresults = (int)results.size();
		assert(nresults == clustersize);
	}

	m.avg_query_ops = 100.0*((double)DBEntry<KeyObject>::n_query_ops/(double)n_clusters/(double)sz);
	m.avg_query_time = (double)querytime.count()/(double)n_clusters;

	cout << " query ops " << dec << setprecision(6) << m.avg_query_ops << "% opers   " 
		 << "query time: " << dec << setprecision(6) << m.avg_query_time << " millisecs" << endl;


	m.avg_memory_used = mtree.memory_usage();
	
	metrics.push_back(m);
	
	mtree.Clear();
	assert(mtree.size() == 0);
}

void do_experiment(const int n, const int n_runs, const int n_entries, const int n_clusters, const int clustersize, const double radius){
	cout << "------------------ " << n << " -------------------" << endl;
	cout << "dataset size, N = " << n_entries  << endl;
	cout << "no. clusters: " << n_clusters << endl;
	cout  << "cluster size " << clustersize << endl;
	cout << "radius: " << radius << endl;
	cout << "averaged across " << n_runs << " runs" << endl;

	vector<perfmetric> metrics;
	for (int i=0;i < n_runs;i++){
		do_run(i+1, n_entries, n_clusters, clustersize, radius, metrics);
	}

	double avg_build_ops = 0;
	double avg_build_time = 0;
	double avg_query_ops = 0;
	double avg_query_time = 0;
	double avg_memory = 0;
	for (struct perfmetric &m : metrics){
		avg_build_ops += m.avg_build_ops/n_runs;
		avg_build_time += m.avg_build_time/n_runs;
		avg_query_ops += m.avg_query_ops/n_runs;
		avg_query_time += m.avg_query_time/n_runs;
		avg_memory += (double)m.avg_memory_used/(double)n_runs;
	}

	cout << "no. runs: " << metrics.size() << endl;
	cout << "avg build:  " << avg_build_ops << "% opers " << avg_build_time << " nanosecs" << endl;
	cout << "avg query:  " << avg_query_ops << "% opers " << avg_query_time << " millisecs" << endl;
	cout << "Memory Usage: " << fixed << setprecision(2) << avg_memory/1000000.0 << "MB" << endl;
	cout << "-------------------------------------------------" << endl << endl;
}

int main(int argc, char **argv){


	cout << "MTree data structure with parameters: " << endl;
	cout << "    no. routes in internal nodes: " << NR << endl;
	cout << "    leaf capacity: " << LC << endl << endl;


	const int N = 9;
	const int n_entries[N] = { 100000, 200000, 400000, 800000, 1000000, 2000000, 4000000, 8000000, 16000000 };
	const int n_clusters = 10;    
	const int clustersize = 10;
	const double radius = 0.04;   
	const int n_runs = 5;

	for (int i=0;i < N;i++){
		do_experiment(i+1, n_runs, n_entries[i], n_clusters, clustersize, radius);
	}

	cout << "Experiments for various radius queries" << endl;
	
	const int M = 1000000;
	const int n_rad = 11;
	const double rad[n_rad] = { 0, 0.02, 0.04, 0.06, 0.08, 0.10, 0.20, 0.40, 0.60, 0.80, 1.0 };

	for (int i=0;i < n_rad;i++){
		do_experiment(i+1, n_runs, M, n_clusters, clustersize, rad[i]);
	}
	
	return 0;
}
