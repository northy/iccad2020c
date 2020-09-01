#include <vector>
#include <tuple>
#include <queue>
#include <map>
#include <set>
#include <utility>
#include <iostream>
#include <iomanip>
#include <functional>
#include <sstream>

#include <cstdint>
#include <ctime>

#include <parser.hpp>
#include <compiler.hpp>

#include <boost/bimap.hpp>

using std::vector;
using std::set;
using std::tuple;
using std::get;
using std::priority_queue;
using std::map;
using std::pair;
using std::cout;

/* Data structures definitions */
enum Logic {False,True,X,Z};

using activation_functor = Logic(uint64_t,uint64_t);

vector<activation_functor*> activation_functors;

struct Process {
    uint64_t id;
    vector<uint64_t> outputs;
    map<uint64_t,map<uint64_t,uint64_t>> delay; //input -> output -> delay
};
vector<Process> processes;

struct Signal {
    uint64_t id;
    Logic value;
    vector<uint64_t> procs;
    uint64_t activator;

    Logic recalc(uint64_t atime) {
        return activation_functors[(*this).activator](atime,(*this).id);
    }
};
vector<Signal> signals;

using signame_bmtype = boost::bimap<uint64_t, std::string>;
using idname_bmtype = boost::bimap<std::string, std::string>;

using generated_stimulus = pair<uint64_t, Logic>; //id, value;
map<uint64_t,set<generated_stimulus>> generated_stimuli; //time -> generated_stimulus
/* Data structures definitions */

/* Queue definitions */
using QueueItem = pair<uint64_t, uint64_t>; //application time, signal

auto compare = [](QueueItem a, QueueItem b) {
    if (a.first==b.first) {
        return a.second > b.second;
    }
    return a.first > b.first;
};

priority_queue<QueueItem, vector<QueueItem>, decltype(compare)> queue(compare);

void queue_add(const uint64_t atime, Process process, const uint64_t signal) {
    for (uint64_t o : process.outputs) {
        queue.push({atime + process.delay[signal][o], o});
    }
}

void queue_dispatch(const QueueItem event) {
    const auto &[atime, sigid] = event;
    Signal &signal = signals[sigid];
    const Logic old = Logic(signal.value);

    signal.value = signal.recalc(atime);

    if (old!=signal.value) {
        generated_stimuli[atime].insert(generated_stimulus(sigid,signal.value));
        for (uint64_t p : signal.procs) {
            queue_add(atime,processes[p],sigid);
        }
    }
}
/* Queue definitions */

map<uint64_t,map<uint64_t,Logic>> stimuli; //Time -> Signal -> value

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(0); std::cout.tie(0); std::cin.tie(0);

    if (argc<2) { 
        cout << "Usage: ./TestSimulation file.vcd\n";
        return 1;
    }

    auto vcd = parser::parsevcd::parse_vcd_file(argv[1]);

    if (!vcd) {
        cout << "Error reading VCD file\n";
        return 1;
    }

    signame_bmtype signame_bimap;
    idname_bmtype idname_bimap;

    /* Example runtime definitions */
    // Circuit:: and = a&b; not = !and;
    
    //TODO: autogenerate
    signame_bimap.insert(signame_bmtype::value_type(0,"a"));
    signame_bimap.insert(signame_bmtype::value_type(1,"b"));
    signame_bimap.insert(signame_bmtype::value_type(2,"and"));
    signame_bimap.insert(signame_bmtype::value_type(3,"not"));

    for (const vcd::Signal s : vcd->signals) {
        idname_bimap.insert(idname_bmtype::value_type(s.id,s.name));
    }
    //TODO: autogenerate
    idname_bimap.insert(idname_bmtype::value_type("%","and"));
    idname_bimap.insert(idname_bmtype::value_type("&","not"));

    for (const vcd::Timestamp t : vcd->timestamps) {
        for (const vcd::Dump d : t.dumps) {
            const uint64_t signal_id = signame_bimap.right.at(idname_bimap.left.at(d.id));
            Logic &l = stimuli[t.time][signal_id];
            const char inp = d.value[0];
            queue.push(QueueItem(t.time,signal_id));
            switch (inp) {
                case '0' :
                    l = Logic(False);
                    break;
                case '1' :
                    l = Logic(True);
                    break;
                case 'x' :
                    l = Logic(X);
                    break;
                case 'z' :
                    l = Logic(Z);
                    break;
            }
        }
    }

    //TODO: autogenerate
    activation_functors = {
        [] (uint64_t atime, uint64_t signal) {
            return stimuli[atime][signal];
        },
        [] (uint64_t atime, uint64_t signal) {
            return Logic(signals[0].value && signals[1].value);
        },
        [] (uint64_t atime, uint64_t signal) {
            return Logic(!signals[2].value);
        },
    };

    //TODO: autogenerate
    signals = {
        {0,False,{0},0},
        {1,False,{0},0},
        {2,X,{1},1},
        {3,X,{},2}
    };

    //TODO: autogenerate
    processes = {
        { //and = a&b
            0, {2}, {
                {uint64_t(0), {
                    {uint64_t(2),uint64_t(0)}
                }},
                {uint64_t(1), {
                    {uint64_t(2),uint64_t(0)}
                }}
            }
        },
        { //not = !and
            1, {3}, {
                {uint64_t(2), {
                    {uint64_t(3),uint64_t(0)}
                }}
            }
        }
    };
    /* Example runtime definitions */

    /* Run */
    while (!queue.empty()) {
        QueueItem event = queue.top();
        queue.pop();
        queue_dispatch(event);
    }
    /* Run */

    /* Output VCD */
    vcd::Vcd new_vcd;

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d/%m/%Y %H:%M:%S");
    new_vcd.date = oss.str();
    
    new_vcd.version = "1.0.0";

    new_vcd.timescale = "1ns";

    new_vcd.scope = {"module logic"};

    for (signame_bmtype::right_map::const_iterator iter = signame_bimap.right.begin(), iend = signame_bimap.right.end(); iter!=iend; ++iter) {
        vcd::Signal s = {"wire",1,idname_bimap.right.at(iter->first),iter->first};
        new_vcd.signals.push_back(s);
    }

    //TODO: autogenerate?
    for (signame_bmtype::right_map::const_iterator iter = signame_bimap.right.begin(), iend = signame_bimap.right.end(); iter!=iend; ++iter) {
        vcd::Dump d = {{'x'},idname_bimap.right.at(iter->first)};
        new_vcd.initial_dump.push_back(d);
    }

    for (map<uint64_t,set<generated_stimulus>>::const_iterator iter = generated_stimuli.begin(), iend = generated_stimuli.end(); iter!=iend; ++iter) {
        vcd::Timestamp t;
        t.time = iter->first;
        for (set<generated_stimulus>::const_iterator setiter = iter->second.begin(), setiend = iter->second.end(); setiter!=setiend; ++setiter) {
            vcd::Dump d;
            d.value = {(char)(setiter->second+48)};
            d.id = idname_bimap.right.at(signame_bimap.left.at(setiter->first));
            t.dumps.push_back(d);
        }
        new_vcd.timestamps.push_back(t);
    }
    new_vcd.timestamps.push_back({new_vcd.timestamps[new_vcd.timestamps.size()-1].time+15,{}});

    compiler::compilevcd::compile_vcd_file(new_vcd, cout);
    /* Output VCD */

    return 0;
}