<h1 style="text-align: center;">MTree</h1>

Distance-based indexing structure.
Elements from the indexed data set are selected at each level of the tree hierarchy to serve as routing objects.
Elements are further sorted into child nodes based on its distance from the routing object at each level.
Those elements that fall under the median distance of all the distances from the routing object go into one
child node.  All those above the median distance go into the other child node.  

## Performance

For the following charts, the indexing structure was tested for various index sizes.
The data set was composed of identical and uniformly independent 16-dimensional real-valued vectors.  
In addition, 10 clusters of 10 vectors each was added to the index for querying tests.
All stats are averaged over 5 separate trial runs.

Build Operations reflect the number of distance calculations performed in building the index from the data,
normalized by the size of the index and expressed as a percentage.  Likewise, the Query Operations are the
average number of distance calculations performed for the 10 separate queries. 

The MEM field indicates the total memory consumed by the data structure.

| N | MEM | Build Opers | Build Time | Query Opers |  Query Time |
|---|-----|-------------|------------|-------------|-------------|
| 100K | 15.5MB | 3173% |  698 ns    |  10.3 %  |  285&mu;s  |   
| 200K | 31.1MB | 3383% |  764 ns    |   5.7 %  |  347&mu;s  |
| 400K | 62.1MB | 3619% |  891 ns    |   4.3 %  |  596&mu;s  |
| 800K | 124MB  | 3832% | 1.04 &mu;s |   3.0 %  |  944&mu;s  |
|  1M  | 155MB  | 3890% | 1.11 &mu;s |   3.0 %  |  1.17 ms  |
|  2M  | 311MB  | 4117% | 1.29 &mu;s |   1.91%  |  1.71 ms  |
|  4M  | 621MB  | 4320% | 1.46 &mu;s |   1.36%  |  2.70 ms  |
|  8M  | 1.24GB | 4553% | 1.66 &mu;s |   0.94%  |  4.15 ms  |
|  16M | 2.48GB | 4773% | 1.82 &mu;s |   0.43%  |  4.22 ms  |


And some query stats for queries set for various radius values
for a constant index size of N = 1M. 


| Radius | Query Opers | Query Times |
|--------|-------------|-------------|
|  0     |  2.61%  |  1.05 ms  |
|  0.02  |  2.46%  |  998&mu;s  |
|  0.04  |  2.81%  |  1.14ms  |
|  0.06  |  2.99%  |  1.21ms  |
|  0.08  |  3.69%  |  1.48ms  |
|  0.10  |  3.38%  |  1.39ms  |
|  0.20  |  7.07%  |  2.85ms  |
|  0.40  |  11.7%  |  5.05ms  |
|  0.60  |  18.3%  |  9.00ms  |
|  0.80  |  25.7%  |  14.5ms  |
|  1.00  |  57.9%  |  18.2ms  |


Use `perfmtree` to generate these results.  The code is available in tests/perfmtree.cpp


## Programming


First, define a custom data object.
The data type must have a constructor, copy constructor, assignment operator and distance function defined.
Like this:

```
struct KeyObject {
	double key[16];
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
```

Next, use an Mtree index:

```
#include <mtree/mtree.hpp>

mt::MTree<KeyObject> mtree;

double valarry[16] = { 0.50, 0.003, ... , 0.02 };

KeyObject key(valarray);           // prepare item to insert
mt::Entry<KeyObject> e(key);
mt::mtree.Insert(e);

int sz = mtree.size();       // get the size of index

mt::KeyObject target(...)    // prepare a target
double target = 0.04;

vector<Entry<KeyObject>> results = mtree.Rangequery(target, radius);
for (auto e : results){
	cout << "Found: id=" << e.id << endl; // vector data of found item in e.key.key 
}

mtree.Clear();
```

## Install

```
cmake .
make
make test
make intall
```
