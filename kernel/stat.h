#ifndef MANIFOLD_KERNEL_STAT_H
#define MANIFOLD_KERNEL_STAT_H

#include <iostream>
#include <iomanip>
#include <math.h>
#include <cassert>
#include <vector>
#include <map>

namespace manifold {
namespace kernel {

typedef unsigned int counter_t;
/*extern Sim_settings *Settings;*/
void fatal_error(const char *fmt, ...);

using namespace std;
//LIST<Stat *> stat_list;

// helper functions used to ensure data integrity for collected statistics
// these are only used for float and double types
template<typename T> inline bool data_ok(T data)
{
    return true;
}

template<> inline bool data_ok<double>(double data)
{
    if (isfinite(data)) {
        return true;
    }
    return false;
}

template<> inline bool data_ok<float>(float data)
{
    if (isfinite(data)) {
        return true;
    }
    return false;
}

// virtual class from which all system statistics will derive
// this guarantees all statistics will provide a name, description, and
// a common interface
template<typename T>
class Stat {
protected:
    string name;
    string desc;

public:
    Stat(const char *n, const char *d);
    virtual ~Stat();
    virtual void print(ostream & out);
    virtual void collect(T val) = 0;
    virtual void clear() = 0;
};

/*
 * Link up the the parent stat class
 */
template<typename T>
class Persistent_stat_interface: public Stat<T> {

public:
    Persistent_stat_interface(const char *n, const char * d);
};

/*
 * Persistent_stat template class declaration
 */

template<typename T>
class Persistent_stat: public Persistent_stat_interface<T> {
protected:
    T value;

public:
    Persistent_stat(const char *n, const char *d);
    void print(ostream & out);
    void collect(T val);

    // the int argument is the c++ way to indicate that the operator is postfix and
    // isn't actually used in the definition
    Persistent_stat& operator++(int);
    Persistent_stat& operator+=(T d);
    Persistent_stat& operator=(T d);
    T get_value();
    void clear();
};

/*
 * Persistent_array_stat template class declaration
 */

template<typename T>
class Persistent_array_stat: public Persistent_stat_interface<T> {
protected:
    unsigned int size;
    std::vector<string *> elem_names;
    std::vector<T> array;

public:
    Persistent_array_stat(const char *d, unsigned int s);
    Persistent_array_stat(const char **n, const char *d, unsigned int s);
    virtual ~Persistent_array_stat();

    virtual void print(ostream & out);
    virtual void collect(T val);
    virtual void clear();

    void collect(T * val);
    T& operator[](int index);
};

template<typename T>
class Persistent_2D_stat: public Persistent_stat_interface<T> {
protected:
    std::map<unsigned int, T> value_map;

    double cumm_sum;
    double cumm_sq_sum;
    counter_t total;

public:
    Persistent_2D_stat(const char *n, const char *d);
    ~Persistent_2D_stat();

    void print(ostream & out);
    void collect(T val);
    void clear();

    void populate(unsigned int index, T value);
    void print_statistics(ostream & out);
    double get_pop_std_dev();
    double get_average();

    void collect(T * val);
};

template<typename T>
class Persistent_histogram_stat: public Persistent_stat_interface<T> {

public:
    unsigned int intervals;
    unsigned int width;
    T start;
    double cumm_sum;
    double cumm_sq_sum;
    counter_t total;
    counter_t max_bin_count;
    std::vector<T> hist_slot;

    Persistent_histogram_stat(const char *n, const char *d, T iv, T wd, T s);

    void print(ostream & out);
    void collect(T m);
    void clear();
    void merge(Persistent_histogram_stat<T> * hist);
    void print_statistics(ostream & out);
    double get_pop_std_dev();
    double get_average();
};

/*
 * Stat template class definition
 */

template<typename T>
Stat<T>::Stat(const char *n, const char *d) :
        name(n), desc(d)
{

    if (desc == "") {
        desc = "NA";
    }
}

template<typename T>
Stat<T>::~Stat()
{
}

template<typename T>
void Stat<T>::print(ostream & out)
{
    if (desc != "NA") {
        out << name << ", " << desc << ", ";
    }
    else {
        out << name << ", ";
    }
}

/*
 * Persistent_stat_interface template class definition
 */
template<typename T>
Persistent_stat_interface<T>::Persistent_stat_interface(const char * n,
        const char * d) :
        Stat<T>::Stat(n, d)
{
}

/*
 * Persistent_stat template class definition
 */
template<typename T>
Persistent_stat<T>::Persistent_stat(const char * n, const char * d) :
        Persistent_stat_interface<T>::Persistent_stat_interface(n, d), value(0)
{
}

template<typename T>
void Persistent_stat<T>::print(ostream & out)
{
    Stat<T>::print(out);
    out << value << std::endl;
}

template<typename T>
void Persistent_stat<T>::collect(T val)
{
    value = val;
}

template<typename T>
void Persistent_stat<T>::clear()
{
    value = 0;
}

template<typename T>
Persistent_stat<T>& Persistent_stat<T>::operator++(int)
{
    value++;
    return *this;
}

template<typename T>
Persistent_stat<T>& Persistent_stat<T>::operator+=(T d)
{
    value += d;
    return *this;
}

template<typename T>
Persistent_stat<T>& Persistent_stat<T>::operator=(T d)
{
    value = d;
    return *this;
}

/* used for the combining statistics into global values */
template<typename T>
T Persistent_stat<T>::get_value()
{
    return value;
}

/*
 * Persistent_array_stat template class definition
 */
template<typename T>
Persistent_array_stat<T>::Persistent_array_stat(const char * d, unsigned int s) :
        Persistent_stat_interface<T>::Persistent_stat_interface(
                "Persistent_array_stat", d), size(s)
{
    elem_names.clear();
    for (unsigned int i = 0; i < size; i++) {
        array.push_back(0);
    }
}

template<typename T>
Persistent_array_stat<T>::Persistent_array_stat(const char ** n, const char * d,
        unsigned int s) :
        Persistent_stat_interface<T>::Persistent_stat_interface(
                "Persistent_array_stat", d), size(s)
{
    elem_names.clear();
    for (unsigned int i = 0; i < size; i++) {
        string * str = new string(n[i]);
        elem_names.push_back(str);
        array.push_back(0);
    }
}

template<typename T>
Persistent_array_stat<T>::~Persistent_array_stat()
{
    while (!elem_names.empty()) {
        string * s = elem_names.back();
        delete s;
        elem_names.pop_back();
    }
}

template<typename T>
void Persistent_array_stat<T>::print(ostream & out)
{
    Stat<T>::print(out);

    out << endl;
    if (elem_names.size()) {
        for (unsigned int i = 0; i < size; i++) {
            out << *elem_names[i];
            if (i != size - 1)
                out << ",";
        }
    }
    else {
        for (unsigned int i = 0; i < size; i++) {
            out << i;
            if (i != size - 1)
                out << ",";
        }
    }
    out << endl;

    for (unsigned int i = 0; i < size; i++) {
        out << array[i];
        if (i != size - 1)
            out << ",";
    }
    out << endl;
}

template<typename T>
void Persistent_array_stat<T>::collect(T * val)
{
    for (int i = 0; i < size; i++) {
        array[i] = val[i];
    }
}

template<typename T>
void Persistent_array_stat<T>::collect(T val)
{
}

template<typename T>
void Persistent_array_stat<T>::clear()
{
    for (unsigned int i = 0; i < size; i++) {
        array[i] = 0;
    }
}

template<typename T>
T& Persistent_array_stat<T>::operator[](int n)
{
    assert((unsigned int)n < size);
    return array[n];
}

/*
 * Persistent_2D_stat template class definition
 */

template<typename T>
Persistent_2D_stat<T>::Persistent_2D_stat(const char * n, const char * d) :
        Persistent_stat_interface<T>::Persistent_stat_interface(n, d)
{
    /** Don't need to do anything to init the value map. */
    value_map.clear();
    cumm_sum = 0;
    cumm_sq_sum = 0;
    total = 0;
}

template<typename T>
Persistent_2D_stat<T>::~Persistent_2D_stat()
{
    value_map.clear();
}

template<typename T>
void Persistent_2D_stat<T>::print(ostream & out)
{
    // need to reimplement.
    fatal_error("unimplemented call");

    /** 2D is a global only stat. */
    if (value_map.size() == 0) {
        return;
    }

    /** Assumes 2D stat is mapped to the chip dimensions! */
    /*if (value_map.size() != (unsigned int) Settings->num_nodes)
     fatal_error("heat_map sizing error?");*/

    Stat<T>::print(out);
    out << endl;
    print_statistics(out);

    /*for(int j = 0; j < Settings->network_y_dimension; j++)
     {
     for(int i = 0; i < Settings->network_x_dimension; i++)
     {
     out << value_map[j * Settings->network_x_dimension + i] << ",";
     }
     out << endl;
     }
     out << endl;*/

    /** Allows stat to be printed multiple times via signal handlers.  */
    clear();
}

template<typename T>
void Persistent_2D_stat<T>::collect(T * val)
{
    //currently un-used
    fatal_error("unimplemented call");
}

template<typename T>
void Persistent_2D_stat<T>::collect(T val)
{
    //currently un-used
    fatal_error("unimplemented call");
}

template<typename T>
void Persistent_2D_stat<T>::clear()
{
    value_map.clear();
    cumm_sum = 0;
    cumm_sq_sum = 0;
    total = 0;
}

template<typename T>
void Persistent_2D_stat<T>::populate(unsigned int index, T value)
{
    assert(value_map[index] == 0 && "double populate on this cell?");

    value_map[index] = value;

    cumm_sq_sum += ((double) value) * value;
    cumm_sum += (double) value;
    total++;
}

template<typename T>
void Persistent_2D_stat<T>::print_statistics(ostream & out)
{
    double average, std_dev;

    average = get_average();
    std_dev = get_pop_std_dev();

    out << "NODES," << "SUM," << "AVG," << "STDDEV" << endl;

    out << total << "," << cumm_sum << "," << average << "," << std_dev << endl;
}

template<typename T>
double Persistent_2D_stat<T>::get_pop_std_dev()
{
    double average = get_average();
    /** population variance = average of the squares - square of the averages
     *  population std dev = sqrt (pop variance)
     */
    return sqrt((((double) cumm_sq_sum) / total) - (average * average));
}

template<typename T>
double Persistent_2D_stat<T>::get_average()
{
    return ((double) cumm_sum) / total;
}

/*
 * Persistent_histogram_stat template class definition
 */
template<typename T>
Persistent_histogram_stat<T>::Persistent_histogram_stat(const char *n,
        const char *d, T iv, T wd, T s) :
        Persistent_stat_interface<T>::Persistent_stat_interface(n, d), intervals(
                iv), width(wd), start(s), cumm_sum(0), cumm_sq_sum(0), total(0), max_bin_count(
                0)
{
    assert(width > 0);

    for (unsigned int i = 0; i < intervals; i++) {
        hist_slot.push_back(0);
    }
}

/* combines the data from this histogram into the contents of the passed histogram */
template<typename T>
void Persistent_histogram_stat<T>::merge(
        Persistent_histogram_stat<T> * histogram)
{
    /* make sure both histograms have the same attributes.  otherwise, merging the
     * information doesn't make any sense
     */
    assert(start == histogram->start);
    assert(intervals == histogram->intervals);
    assert(width == histogram->width);

    for (unsigned int i = 0; i < intervals; i++) {
        hist_slot[i] += histogram->hist_slot[i];
    }

    if (histogram->max_bin_count > max_bin_count) {
        max_bin_count = histogram->max_bin_count;
    }
    cumm_sq_sum += histogram->cumm_sq_sum;
    cumm_sum += histogram->cumm_sum;
    total += histogram->total;
}

template<typename T>
void Persistent_histogram_stat<T>::collect(T m)
{
    cumm_sum += (double) m;
    cumm_sq_sum += ((double) m) * m;
    m = m - start;

    if (!data_ok(m))
        return;

    for (unsigned int i = 0; i < intervals; i++) {
        if ((unsigned int) m < width) {
            hist_slot[i]++;
            total++;

            if ((counter_t) hist_slot[i] > max_bin_count) {
                max_bin_count = hist_slot[i];
            }
            return;
        }
        else {
            m = m - width;
        }
    }

    total++;
    hist_slot[intervals - 1]++;
}

template<typename T>
void Persistent_histogram_stat<T>::clear()
{
    hist_slot.clear();
    cumm_sum = 0;
    cumm_sq_sum = 0;
    total = 0;
    for (unsigned int i = 0; i < intervals; i++) {
        hist_slot.push_back(0);
    }
}

template<typename T>
void Persistent_histogram_stat<T>::print(ostream & out)
{
    T tmp_range = start;
    int sw = (int) ceil(log(max_bin_count) / log(10));
    counter_t i;
    float cumm_before = 0;
    float cumm_after = 0;
    float perc;

    Stat<T>::print(out);
    out << "EVENTNUM:" << total << ",";
    out << "INTERVAL:" << intervals << ",";
    out << "WIDTH:" << width << ",";
    out << "START:" << start << endl;

    print_statistics(out);

    if (1/*Settings->report_output == OUTPUT_FMT_CSV*/) {

        for (i = 0; i < (counter_t) intervals - 1; i++) {
            out << i * width << ",";
        }
        out << "INF" << endl;

        for (i = 0; i < (counter_t) intervals - 1; i++) {
            out << hist_slot[i] << ",";
        }
        out << hist_slot[intervals - 1] << endl;
    }
    else {
        for (i = 0; i < (counter_t) intervals; i++) {
            cumm_before = cumm_after;

            out << setprecision(2) << setiosflags(ios::fixed);
            out << setw(6) << "Bin " << setw(3) << i << ", " << setw(4)
                    << tmp_range << " to ";

            if (i == intervals - 1) {
                out << setw(4) << "INF" << ":";
            }
            else {
                out << setw(4) << tmp_range + width << ":";
            }

            cumm_after += ((float) hist_slot[i] / (float) total * 100);

            out << setw(sw) << hist_slot[i] << " (" << setw(6) << cumm_before
                    << ", " << setw(6) << cumm_after << ") ";

            perc = ((float) hist_slot[i] / (float) total) * 100;
            if (perc >= 1.0) {
                for (int j = 0; j < (int) perc; j++) {
                    out << "*";
                }
            }
            out << endl << setprecision(4);
            tmp_range += width;

        }
        out << endl;
    }
}

template<typename T>
void Persistent_histogram_stat<T>::print_statistics(ostream & out)
{
    double average, std_dev;

    average = get_average();
    std_dev = get_pop_std_dev();

    out << "POPULATION SIZE, " << "SUM, " << "AVG, " << "STDDEV" << endl;

    out << total << ", " << cumm_sum << ", " << average << ", " << std_dev
            << endl;
}

template<typename T>
double Persistent_histogram_stat<T>::get_pop_std_dev()
{
    double average = get_average();
    /** population variance = average of the squares - square of the averages
     *  population std dev = sqrt (pop variance)
     */
    return sqrt((((double) cumm_sq_sum) / total) - (average * average));
}

template<typename T>
double Persistent_histogram_stat<T>::get_average()
{
    if (total == 0)
        return 0;

    return ((double) cumm_sum) / total;
}

/* Sampled statistics classes */
template<typename T>
class Sampled_stat_interface: public Stat<T> {
public:
    Sampled_stat_interface(const char *n, const char * d);
};

template<typename T>
class Sampled_stat: public Sampled_stat_interface<T> {

public:
    std::vector<T> data;

    Sampled_stat(const char *n, const char *d);

    void print(ostream & out);
    void print_statistics(ostream & out);
    void collect(T datapt);
    void clear();

    T& operator[](int index);
};

template<typename T>
Sampled_stat_interface<T>::Sampled_stat_interface(const char * n,
        const char * d) :
        Stat<T>::Stat(n, d)
{
}

template<typename T>
Sampled_stat<T>::Sampled_stat(const char *n, const char *d) :
        Sampled_stat_interface<T>::Sampled_stat_interface(n, d)
{
}

template<typename T>
void Sampled_stat<T>::collect(T val)
{
    data.push_back(val);
}

template<typename T>
void Sampled_stat<T>::clear()
{
    data.clear();
}

template<typename T>
T& Sampled_stat<T>::operator[](int n)
{
    assert((unsigned int)n < data.size());
    return data[n];
}

template<typename T>
void Sampled_stat<T>::print_statistics(ostream & out)
{
    int size;
    unsigned int index;
    double average;
    double sum;
    double my_95_bounds;

    size = 0;
    average = 0;
    sum = 0;
    my_95_bounds = 0;

    for (index = 0; index < data.size(); index++) {
        if (data_ok(data[index])) {
            average = average + data[index];
            size++;
        }
    }
    sum = average;
    average = average / size;

    for (index = 0; index < data.size(); index++) {
        if (data_ok(data[index])) {
            my_95_bounds += pow((data[index] - average), 2);
        }
    }
    my_95_bounds = (sqrt(my_95_bounds / (size - 1)) / sqrt(size)) * 1.96;

    out << Stat<T>::name << "statistics,";
    out << "TOTAL," << "SUM," << "AVG," << "95%% CI," << "95%% LOWER BOUND,"
            << "95%% UPPER BOUND" << endl;

    out << size << "," << sum << "," << average << "," << my_95_bounds << ","
            << (average - my_95_bounds) << "," << (average + my_95_bounds)
            << endl;
}

template<typename T>
void Sampled_stat<T>::print(ostream & out)
{
    Stat<T>::print(out);

    unsigned int index;

    for (index = 0; index < data.size(); index++) {
        out << data[index] << ",";
    }
    out << endl;
}

/* the simulator doesn't have sampling, so sampled statistic implementation will be deferred
 * until that time
 */
//TODO come back and fill in the sampled statistics
/*
 template<typename T>
 class Sampled_stat_interface : public Stat {
 };


 template<typename T>
 class Sampled_stat: public Sampled_stat_interface<T> {
 private:
 LIST<T> data;

 public:
 Sampled_stat (const char *n, const char *d);

 void collect ( T datapt );
 void print ( ostream & out );
 void print_statistics ( ostream & out );
 void clear ();
 };





 template<typename T>
 class SampledPersistent_array_stat: public Persistent_array_stat<T> {
 public:
 VECTOR< VECTOR<T> > data_vec;

 SampledPersistent_array_stat (const char ** n, const char *d, unsigned int size);

 void print ( ostream & out );
 void print_statistics ( ostream & out );
 void collect ( T * val );
 void clear ();
 };




 template<typename T>
 Sampled_stat<T>::Sampled_stat(const char *n, const char *d) :
 Stat<T>::Stat(n, d) {
 data.clear();
 }

 template<typename T>
 void Sampled_stat<T>::collect(T val) {
 if (!data_ok(val))
 return;

 data.push_back(val);
 }

 template<typename T>
 void Sampled_stat<T>::clear() {
 data.clear();
 }

 template<typename T>
 void Sampled_stat<T>::print_statistics(ostream & out) {
 int size;
 double average;
 double sum;
 double my_95_bounds;
 typedef typename LIST<T>::iterator tIter;

 size = data.size();
 average = 0;
 sum = 0;
 my_95_bounds = 0;

 for(tIter iter = data.begin(); iter != data.end(); iter++)
 {
 average = average + *iter;
 }
 sum = average;
 average = average / size;

 for(tIter iter = data.begin(); iter != data.end(); iter++)
 {
 my_95_bounds += pow((*iter - average), 2);
 }
 my_95_bounds = (sqrt(my_95_bounds/(size-1))/sqrt(size)) * 1.96;

 out << Stat<T>::name << "statistics,";
 out << "TOTAL," << "SUM," << "AVG,"
 << "95%% CI," << "95%% LOWER BOUND,"
 << "95%% UPPER BOUND" << endl;

 out << size << "," << sum << "," << average << ","
 << my_95_bounds << "," << (average - my_95_bounds) << ","
 << (average + my_95_bounds ) << endl;
 }

 template<typename T>
 void Sampled_stat<T>::print(ostream & out) {
 Stat<T>::print(out);

 typedef typename LIST<T>::iterator tIter;
 for(tIter iter = data.begin(); iter != data.end(); iter++)
 {
 if ( iter != data.begin() )
 {
 out << "," << *iter;
 }
 else
 {
 out << *iter;
 }
 }
 out << endl;
 }


 template<typename T>
 AggregateStat<T>::AggregateStat(const char * n, const char * d) :
 SingleStat<T>::SingleStat(n, d) {
 SingleStat<T>::value = 0;
 }

 template<typename T>
 void AggregateStat<T>::collect(T val) {
 if (!data_ok(val))
 return;

 SingleStat<T>::value += val;
 }

 template<typename T>
 SingleStat<T>::SingleStat(const char * n, const char * d) :
 Stat<T>::Stat(n, d) {
 }

 template<typename T>
 void SingleStat<T>::print(ostream & out) {

 Stat<T>::print(out);
 out << "," << value << endl;
 }

 template<typename T>
 void SingleStat<T>::collect(T new_val) {
 if (!data_ok(new_val))
 return;

 value = new_val;
 }

 template<typename T>
 void SingleStat<T>::clear() {
 value = 0;
 }

 template<typename T>
 Persistent_array_stat<T>::Persistent_array_stat(const char ** n, const char * d, unsigned int s) :
 names(NULL), desc(d), size(s) {
 names = new string[size];
 for (unsigned int i = 0; i < size; i++) {
 names[i] = n[i];
 }
 }

 template<typename T>
 Persistent_array_stat<T>::~Persistent_array_stat() {
 delete[] names;
 }

 template<typename T>
 void Persistent_array_stat<T>::print(ostream & out) {
 for (unsigned int i = 0; i < size; i++) {
 out << names[i];
 if (i != size - 1) {
 out << ",";
 }
 }
 }

 template<typename T>
 SampledPersistent_array_stat<T>::SampledPersistent_array_stat(const char ** n, const char * d,
 unsigned int size) :
 Persistent_array_stat<T>::Persistent_array_stat(n, d, size) {

 for (unsigned int i = 0; i < Persistent_array_stat<T>::size; i++) {
 VECTOR<T> tmp_vec;
 data_vec.push_back( tmp_vec );
 }
 }

 template<typename T>
 void SampledPersistent_array_stat<T>::collect(T * val) {
 typedef typename VECTOR< VECTOR<T> >::iterator vviter;
 unsigned int idx = 0;

 for ( vviter i = data_vec.begin(); i != data_vec.end(); i++ )
 {
 if (!data_ok(val[idx]))
 {
 return;
 }
 }

 for ( vviter i = data_vec.begin(); i != data_vec.end(); i++ )
 {
 (*i).push_back( val[idx] );
 idx++;
 }
 }

 template<typename T>
 void SampledPersistent_array_stat<T>::clear() {
 typedef typename VECTOR< VECTOR<T> >::iterator vvIter;

 for( vvIter i = data_vec.begin(); i != data_vec.end(); i++ )
 {
 (*i).clear();
 }

 }

 template<typename T>
 void SampledPersistent_array_stat<T>::print(ostream & out) {
 Persistent_array_stat<T>::print(out);

 unsigned int xlen, ylen, x, y;

 xlen = data_vec.size();
 ylen = (*data_vec.begin()).size();

 for (y = 0; y < ylen; y++) {
 for (x = 0; x < xlen; x++) {
 out << data_vec[x][y] << ",";
 }
 out << endl;
 }
 }

 template<typename T>
 void SampledPersistent_array_stat<T>::print_statistics(ostream & out) {

 unsigned int xlen, ylen, x, y;
 double average;
 double sum;
 double my_95_bounds;

 xlen = data_vec.size();
 ylen = (*data_vec.begin()).size();

 for (x = 0; x < xlen; x++) {
 average = 0;
 sum = 0;
 my_95_bounds = 0;

 for (y = 0; y < ylen; y++) {
 average += data_vec[x][y];
 }
 sum = average;
 average = average / ylen;

 for (y = 0; y < ylen; y++) {
 my_95_bounds += pow((data_vec[x][y] - average), 2);
 }
 my_95_bounds = (sqrt(my_95_bounds / (ylen - 1)) / sqrt(ylen)) * 1.96;

 out << Persistent_array_stat<T>::names[x] << " statistics" << endl;
 out << "TOTAL," << "SUM," << "AVG," << "95%% CI,"
 << "95%% LOWER BOUND," << "95%% UPPER BOUND" << endl;

 out << ylen << "," << sum << "," << average << "," << my_95_bounds
 << "," << (average - my_95_bounds) << "," << (average
 + my_95_bounds) << endl;
 }
 }
 */

}// namespace kernel
} // namespace manifold

#endif //MANIFOLD_KERNEL_STAT_H
