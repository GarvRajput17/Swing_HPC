
// SwingWorker.cc - skeleton implementation
//

//
#include "SwingWorker.h"
#include <iostream>

using namespace SST;

SwingWorker::SwingWorker(ComponentId_t id, Params& params) :
    Component(id),
    out("", 1, 0, Output::STDOUT),
    routerLink(nullptr),
    rank(0),
    numRanks(16),
    vectorSize(1024),
    stepsToRun(3),
    currentStep(0)
{
    // Read params (if present)
    if (params.find("rank") != params.end()) rank = params.find<int>("rank");
    if (params.find("numRanks") != params.end()) numRanks = params.find<int>("numRanks");
    if (params.find("vectorSize") != params.end()) vectorSize = params.find<uint64_t>("vectorSize");
    if (params.find("steps") != params.end()) stepsToRun = params.find<int>("steps");

    // Default to bandwidth-optimal
    algorithm = std::make_unique<swing::SwingAlgorithm>(swing::SwingAlgorithm::Variant::BANDWIDTH_OPTIMAL);

    // Create link to router (assumes one link named 'port' in python input)
    routerLink = configureLink("port", "1ns", new Event::Handler<SwingWorker>(this, &SwingWorker::clockTick));
    if (!routerLink) {
        out.output("Warning: router link not configured. This worker will run in standalone mode.\n");
    }

    // Register a clock for driving steps (1GHz default)
    registerClock("1GHz", new Clock::Handler<SwingWorker>(this, &SwingWorker::clockTick));
}

void SwingWorker::init(unsigned int phase) {
    // Nothing for now
}

void SwingWorker::setup() {
    out.output("SwingWorker[%d] setup. rank=%d numRanks=%d vectorSize=%llu steps=%d\n", getId(), rank, numRanks, (unsigned long long)vectorSize, stepsToRun);

    // Pre-schedule step 0
    scheduleStep(0);
}

void SwingWorker::finish() {
    out.output("SwingWorker[%d] finished.\n", getId());
}

bool SwingWorker::clockTick(Cycle_t cycle) {
    // If there are ops scheduled for currentStep, send them and advance.
    if (!stepOps.empty()) {
        out.output("Step %d: issuing %zu sends\n", currentStep, stepOps.size());
        for (const auto &op : stepOps) {
            sendComm(op);
        }
        stepOps.clear();
        currentStep++;
        if (currentStep < stepsToRun) {
            scheduleStep(currentStep);
        } else {
            out.output("SwingWorker done with %d steps\n", stepsToRun);
        }
    }
    return false; // don't keep clock if no further work; returning true schedules next clock
}

void SwingWorker::scheduleStep(int s) {
    out.output("Schedule step %d\n", s);
    // Use the SwingAlgorithm's pi() to compute peers for this step
    // For simplicity, we generate comm ops for each rank as if this component
    // held the local rank; in a real run each component would be instantiated
    // with its own rank value and schedule only its own sends.
    stepOps.clear();
    int p = numRanks;
    for (int r = 0; r < p; ++r) {
        int peer = algorithm->pi(r, s, p);
        CommOp op{r, peer, static_cast<uint64_t>(vectorSize / (1 << (s+1)))};
        // Only record ops where src == this rank (real component would only send its own)
        if (r == rank) stepOps.push_back(op);
    }
    out.output("SwingWorker[%d] prepared %zu ops for step %d\n", rank, stepOps.size(), s);
}

void SwingWorker::sendComm(const CommOp &op) {
    out.output("SwingWorker[%d] send -> dst=%d size=%llu bytes\n", rank, op.dst, (unsigned long long)op.size_bytes);
    if (routerLink) {
        // Very lightweight placeholder event: we send a raw SST::Event with no payload.
        // For a real simulation, implement a serializable Event that carries size, tag, and step.
        Event* ev = new Event();
        routerLink->send(ev);
    }
}
