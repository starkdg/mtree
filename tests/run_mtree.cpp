#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <random>
#include <vector>
#include <chrono>
#include <cassert>
#include <ratio>
#include "mtree/mtree.hpp"

using namespace std;
using namespace mt;

static random_device m_rd;
static mt19937_64 m_gen(m_rd());
static uniform_int_distribution<uint64_t> m_distrib(0);

static long long m_id = 0;
static long long g_id = 100000000;


const int NR = 2;
const int LC = 100;

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
	const double distance(const KeyObject other)const{
		return __builtin_popcountll(key^other.key);
	}
};

struct perfmetric {
	double avg_build_ops;
	double avg_build_time;
	double avg_query_ops;
	double avg_query_time;
	size_t avg_memory_used;
};


int generate_data(vector<Entry<KeyObject>> &entries, const int N){

	for (int i=0;i < N;i++){
		Entry<KeyObject> entry = { .id = m_id++, .key = KeyObject(m_distrib(m_gen)) };
		entries.push_back(entry);
	}

	return entries.size();
}

int generate_cluster(vector<Entry<KeyObject>> &entries, const uint64_t center,
					 const int n, const int radius){
	uniform_int_distribution<int> radius_distr(1, radius);
	uniform_int_distribution<int> bitindex_distr(0, 63);
		
	uint64_t mask = 0x01;

	entries.push_back({ .id = g_id++, .key = KeyObject(center) });
	for (int i=0;i < n-1;i++){
		uint64_t val = center;
		if (radius > 0){
			int dist = radius_distr(m_gen);
			for (int j=0;j < dist;j++){
				val ^= (mask << bitindex_distr(m_gen));
			}
		}
		entries.push_back({ .id = g_id++, .key = KeyObject(val) });
	}
	return n;
}

void do_run(const int index, const int n_entries, const int n_clusters,
			const int cluster_size, const int radius, vector<struct perfmetric> &metrics){
	m_id = 1;
	g_id = 100000000;

	MTree<KeyObject,NR,LC>  mtree;

	int sz;
	chrono::duration<double, nano> total(0);
	RoutingObject<KeyObject>::n_build_ops = 0;

	vector<Entry<KeyObject>> entries;
	generate_data(entries, n_entries);
	
	auto s = chrono::steady_clock::now();
	for (auto &e : entries){
		mtree.Insert(e);
	}
	auto e = chrono::steady_clock::now();
	total += (e - s);
	sz = mtree.size();

	assert(sz == n_entries);

	struct perfmetric m;
	
	m.avg_build_ops = 100.0*((double)RoutingObject<KeyObject>::n_build_ops/(double)sz); 
	m.avg_build_time = total.count()/(double)sz;
	
	cout << "(" << index << ") build tree: " << setw(10) << setprecision(6) << m.avg_build_ops << "% opers "
		 << setw(10) << setprecision(6) << m.avg_build_time << " nanosecs ";

	uint64_t centers[n_clusters];
	for (int i=0;i < n_clusters;i++){
		centers[i] = m_distrib(m_gen);

		vector<Entry<KeyObject>> cluster;
		generate_cluster(cluster, centers[i], cluster_size, radius);
		assert((int)cluster.size() == cluster_size);

		for (auto &e : cluster){
			mtree.Insert(e);
		}
		
		sz = mtree.size();
		assert(sz == n_entries + cluster_size*(i+1));
	}

	DBEntry<KeyObject>::n_query_ops = 0;
	chrono::duration<double, milli> querytime(0);
	for (int i=0;i < n_clusters;i++){

		auto s = chrono::steady_clock::now();
		vector<Entry<KeyObject>> results = mtree.RangeQuery(KeyObject(centers[i]), radius);
		auto e = chrono::steady_clock::now();
		querytime += (e - s);

		int nresults = (int)results.size();
		assert(nresults >= cluster_size);
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

void do_experiment(const int idx, const int n_runs, const int n_entries, const int n_clusters, const int cluster_size, const int radius){

	cout << "-----------experiment " << idx << "--------------" << endl;
	cout << "N = " << n_entries << endl;
	cout << "across " << n_runs << " runs" << endl;
	cout << "no. clusters: " << n_clusters <<  " of size "
		 << cluster_size << " with radius = " << radius << endl << endl;

	vector<perfmetric> metrics;
	for (int i=0;i < n_runs;i++){
		do_run(i, n_entries, n_clusters, cluster_size, radius, metrics);
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

	cout << "avg build:  " << avg_build_ops << "% opers " << avg_build_time << " nanosecs" << endl;
	cout << "avg query:  " << avg_query_ops << "% opers " << avg_query_time << " millisecs" << endl;
	cout << "Memory Usage: " << fixed << setprecision(2) << avg_memory/1000000.0 << "MB" << endl;

	cout << "---------------------------------------------------" << endl << endl;
}

int main(int argc, char **argv){

	const int n_runs = 5;
	const int n_experiments = 9;
	const int n_entries[n_experiments] = { 100000, 200000, 400000, 800000, 1000000, 2000000, 4000000, 8000000, 16000000};
	const int n_clusters = 10;
	const int cluster_size = 10;
	const int radius = 5;

	cout << "MTree data structure with parameters: " << endl;
	cout << "    no. routes in internal nodes: " << NR << endl;
	cout << "    leaf capacity: " << LC << endl;

	cout << endl << endl;
	
	cout << "Experiments for varying index size" << endl << endl;

	for (int i=0;i < n_experiments;i++){
		do_experiment(i+1, n_runs, n_entries[i], n_clusters, cluster_size, radius);
	}


	const int N = 4000000;
	const int n_rad = 7;
	const int rad[n_rad] = { 0, 2, 4, 6, 8, 10, 12};

	cout << "Experiments for varying range search" << endl;
	cout << "Index size = " << N << endl << endl;

	for (int i=0;i < n_rad;i++){
		do_experiment(i+1, n_runs, N, n_clusters, cluster_size, rad[i]);
	}
	
	return 0;
}
