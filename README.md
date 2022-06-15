# MTree

Distance-based indexing structure.  Elements from the
indexed data set are selected at each level of the tree
to serve as routing objects.  Elements are further sorted
into child nodes based on its distance from the routing
object.  The median threshold is selected from the minimum
and maximum distances between the routing object and the
data set of itms indexed to that node.  


## Performance

You can test the performance with `runmtree` test program.

The tests involve loaded up with uniformly randomly distributed
data points.  The number of such data points vary from 100K to 16M.
The build operations are simply the average number of distance
computations needed to insert each item.  Likewise, the query operations
are the average number of distance computations per query.  Both are
normalized to the total number of indexed items and expressed as a
percentage.  Ten clusters of ten datapoints are inserted, each being
a cluster of points with a maximum radius of 10.  The queries performed
in the test are queries of these clusters.  All stats in this chart
reflect an average of 5 different runs of the tests, where each run the
tree is constructed, queried against, and taken down. 


|  Size  |  MEM   |  build opers  |  build time  |  query opers  |  query time  |  
|--------|--------|---------------|--------------|---------------|--------------|
| 100K   |  2.6MB  |  2851%  |  251 ns  |  84.2%  |  0.75 ms  |
| 200K   |  5.3MB  |  3060%  |  268 ns  |  82.8%  |  1.8 ms  |
| 400K   |  10.5MB  |  3267%  |  297 ns  |  81.3%  |  4.0 ms  |
| 800K   |  21.0MB  |  3470%  |  340 ns  |  79.9%  |  8.6 ms  |
|   1M   |  26.2MB  |  3531%  |  360 ns  |  79.4%  |  11.1 ms  |
|   2M   |  52.5MB  |  3739%  |  446 ns  |  77.8%  |  23.1 ms  |
|   4M   |  105MB  |  3946%  |  585 ns  |  76.0%  |  49.6 ms  |
|   8M   |  210MB  |  4151%  |  768 ns  |  74.2%  |  107 ms  |
|  16M   |  420MB  |  4358%  |  971 ns  |  72.3%  |  228 ms  |


That reflects a very high radius.  Here's one using a radius of half of that
(radius=5), leaving out the MEM and build times, since they are the same as above. 


|  Size  |  query opers  | query time  |
|--------|---------------|-------------|
|  100K  |  47.1%  |  0.79 ms  |
|  200K  |  44.5%  |  1.71 ms  |
|  400K  |  42.3%  |  3.69 ms  |
|  800K  |  39.5%  |  7.58 ms  |
|  1M    |  38.7%  |  9.55 ms  |
|  2M    |  36.2%  |  19.19 ms  |
|  4M    |  33.7%  |  39.14 ms  |
|  8M    |  31.1%  |  80.10 ms  |
| 16M    |  28.6%  |  161.17 ms |


Finally, a performance chart for varying range searches, where the radius
ranges from 0 to 12.  The size of the data set is a constant 4 million. 

| Radius | query opers | query time |
|--------|-------------|------------|
|   0    |    0.86%    |   5.30 ms  |
|   2    |    9.00%    |  18.43 ms  |
|   4    |    24.3%    |  33.19 ms  |
|   6    |    43.3%    |  44.38 ms  |
|   8    |    61.4%    |  50.20 ms  |
|  10    |    76.1%    |  30.38 ms  |
|  12    |    86.3%    |  48.25 ms  |



## Install

```
cmake .
make
make test
make intall
```
