#include <vector>
#include <cstdint>
#include <tuple>
#include <queue>
#include <map>
#include <utility>
#include <iostream>
#include <functional>

using std::vector;
using std::tuple;
using std::priority_queue;
using std::map;
using std::pair;
using std::get;
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
        cout << "At time " << atime << " signal " << sigid << " became " << signal.value << '\n';
        for (uint64_t p : signal.procs) {
            queue_add(atime,processes[p],sigid);
        }
    }
}
/* Queue definitions */

map<uint64_t,map<uint64_t,Logic>> stimuli; //Time -> Signal -> value

int main() {
    /* Example runtime definitions */
    stimuli = {
        {uint64_t(0), {
            {uint64_t(0), Logic(1)}
        }},
        {uint64_t(15), {
            {uint64_t(1), Logic(1)}
        }},
        {uint64_t(30), {
            {uint64_t(0), Logic(0)}
        }}
    };

    activation_functors = {
        [] (uint64_t atime, uint64_t signal) {
            return stimuli[atime][signal];
        },
        [] (uint64_t atime, uint64_t signal) {
            return Logic(signals[0].value && signals[1].value);
        }
    };

    signals = {
        {0,False,{0},0},
        {1,False,{0},0},
        {2,False,{},1}
    };

    processes = {
        {
            0, {2}, {
                {uint64_t(0), {
                    {uint64_t(2),uint64_t(0)}
                }},
                {uint64_t(1), {
                    {uint64_t(2),uint64_t(0)}
                }}
            }
        }
    };
    
    queue.push(QueueItem(0,0));
    queue.push(QueueItem(15,1));
    queue.push(QueueItem(30,0));
    /* Example runtime definitions */

    while (!queue.empty()) {
        QueueItem event = queue.top();
        queue.pop();
        queue_dispatch(event);
    }

    return 0;
}