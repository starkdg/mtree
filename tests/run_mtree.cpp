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

static random_device m_rd;
static mt19937_64 m_gen(m_rd());
static uniform_int_distribution<uint64_t> m_distrib(0);

static long long m_id = 0;
static long long g_id = 100000000;

const int n_runs = 5;

const int n_entries = 1000000;
const int n_iters = 10;
const int n_clusters = 10;
const int cluster_size = 10;
const double radius = 10;

const int NR = 24;
const int LC = 2000;

struct KeyObject {
	static unsigned long n_ops;
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
		KeyObject::n_ops++;
		return __builtin_popcountll(key^other.key);
	}
};

unsigned long KeyObject::n_ops = 0;

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

int generate_cluster(vector<Entry<KeyObject>> &entries, uint64_t center, int N, int max_radius){
	static uniform_int_distribution<int> radius_distr(1, max_radius);
	static uniform_int_distribution<int> bitindex_distr(0, 63);
		
	uint64_t mask = 0x01;

	entries.push_back({ .id = g_id++, .key = KeyObject(center) });
	for (int i=0;i < N-1;i++){
		uint64_t val = center;
		int dist = radius_distr(m_gen);
		for (int j=0;j < dist;j++){
			val ^= (mask << bitindex_distr(m_gen));
		}
		entries.push_back({ .id = g_id++, .key = KeyObject(val) });
	}
	return N;
}

void do_run(int index, vector<struct perfmetric> &metrics){
	m_id = 1;
	g_id = 100000000;

	MTree<KeyObject,NR,LC>  mtree;

	int sz;
	chrono::duration<double> total(0);
	KeyObject::n_ops = 0;
	for (int i=0;i < n_iters;i++){
		vector<Entry<KeyObject>> entries;
		generate_data(entries, n_entries);
	
		auto s = chrono::steady_clock::now();
		for (auto &e : entries){
			mtree.Insert(e);
		}
		auto e = chrono::steady_clock::now();
		total += (e - s);
		sz = mtree.size();

		assert(sz == n_entries*(i+1));
	}

	struct perfmetric m;
	
	m.avg_build_ops = 100.0*((double)KeyObject::n_ops/(double)sz); 
	m.avg_build_time = total.count();
	
	cout << "(" << index << ") build tree: " << setw(10) << setprecision(6) << m.avg_build_ops << "% opers "
		 << setw(10) << setprecision(6) << m.avg_build_time << " secs ";

	uint64_t centers[n_clusters];
	for (int i=0;i < n_clusters;i++){
		centers[i] = m_distrib(m_gen);

		vector<Entry<KeyObject>> cluster;
		generate_cluster(cluster, centers[i], cluster_size, radius);
		assert(cluster.size() == cluster_size);
		for (auto &e : cluster){
			mtree.Insert(e);
		}
		
		sz = mtree.size();
		assert(sz == n_entries*n_iters + cluster_size*(i+1));
	}

	KeyObject::n_ops = 0;
	chrono::duration<double, milli> querytime(0);
	for (int i=0;i < n_clusters;i++){

		auto s = chrono::steady_clock::now();
		vector<Entry<KeyObject>> results = mtree.RangeQuery(KeyObject(centers[i]), radius);
		auto e = chrono::steady_clock::now();
		querytime += (e - s);

		int nresults = (int)results.size();
		assert(nresults >= cluster_size);
	}

	m.avg_query_ops = 100.0*((double)KeyObject::n_ops/(double)sz/(double)n_clusters);
	m.avg_query_time = (double)querytime.count()/(double)n_clusters;

	cout << " query ops " << dec << setprecision(6) << m.avg_query_ops << "% opers   " 
		 << "query time: " << dec << setprecision(6) << m.avg_query_time << " millisecs" << endl;


	m.avg_memory_used = mtree.memory_usage();
	
	metrics.push_back(m);
	
	mtree.Clear();
	assert(mtree.size() == 0);
}



int main(int argc, char **argv){


	cout << "N = " << n_entries << "x" << n_iters << endl;
	cout << "across " << n_runs << " runs" << endl;
	cout << "no. clusters: " << n_clusters <<  " of size "
		 << cluster_size << " with radius = " << radius << endl;

	cout << "MTree data structure with parameters: " << endl;
	cout << "    no. routes in internal nodes: " << NR << endl;
	cout << "    leaf capacity: " << LC << endl;

	cout << endl << endl;
	
	vector<perfmetric> metrics;
	for (int i=0;i < n_runs;i++){
		do_run(i, metrics);
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
	
	cout << "avg build:  " << avg_build_ops << "% opers " << avg_build_time << " seconds" << endl;
	cout << "avg query:  " << avg_query_ops << "% opers " << avg_query_time << " milliseconds" << endl;
	cout << "Memory Usage: " << fixed << setprecision(2) << avg_memory/1000000.0 << "MB" << endl;
	
	cout << endl << "Done." << endl;


	
	return 0;
}
