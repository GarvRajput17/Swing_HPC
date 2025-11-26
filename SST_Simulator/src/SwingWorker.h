#pragma once
// SwingWorker.h - SST component skeleton for running Swing schedules
//
// This is a lightweight skeleton that shows how to plug the SwingAlgorithm
// scheduling into an SST Component. It is intentionally generic and may
// require small changes to compile against your SST version.
//
// Author: Auto-generated skeleton (adapt as needed)
//
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/event.h>
#include <sst/core/output.h>
#include <vector>
#include <memory>
#include "SwingAlgorithm.h" // algorithm header you provided

using namespace SST;

struct CommOp {
    int src;
    int dst;
    uint64_t size_bytes;
};

class SwingWorker : public Component {
public:
    SwingWorker(ComponentId_t id, Params& params);
    void init(unsigned int phase);
    void setup();
    void finish();

private:
    bool clockTick(Cycle_t cycle);
    Link* routerLink; // connect to local router
    Output out;
    int rank;
    int numRanks;
    uint64_t vectorSize;
    int stepsToRun;
    std::unique_ptr<swing::SwingAlgorithm> algorithm;

    // internal bookkeeping
    int currentStep;
    std::vector<CommOp> stepOps;

    // helper
    void scheduleStep(int s);
    void sendComm(const CommOp &op);
};
