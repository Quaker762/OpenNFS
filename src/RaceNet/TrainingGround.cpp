//
// Created by Amrik on 28/10/2018.
//

#include "TrainingGround.h"

TrainingGround::TrainingGround(uint16_t populationSize, uint16_t nGenerations, uint16_t nTicks, shared_ptr<ONFSTrack> training_track) {
    LOG(INFO) << "Beginning GA evolution session. Population Size: " << populationSize << " nGenerations: "
              << nGenerations << " Track: " << training_track->name << " (" << ToString(training_track->tag) << ")";

    this->training_track = training_track;
    physicsEngine.registerTrack(this->training_track);

    InitialiseAgents(populationSize);
    std::vector<std::vector<int>> trainedAgentFitness = TrainAgents(nGenerations, nTicks);

    LOG(INFO) << "Saving best agent network to " << BEST_NETWORK_PATH;
    car_agents[trainedAgentFitness[0][0]]->carNet.net.saveNetworkParams(BEST_NETWORK_PATH.c_str());

    LOG(INFO) << "Done";
}

void TrainingGround::Mutate(RaceNet &toMutate) {
    unsigned long layerToMutate = rand() % toMutate.net.W.size();

    auto &m = toMutate.net.W[layerToMutate];

    int h = m.getHeight();
    int w = m.getWidth();

    int xNeuronToMutate = rand() % h;
    int yNeuronToMutate = rand() % w;

    m.put(xNeuronToMutate, yNeuronToMutate, m.get(xNeuronToMutate, yNeuronToMutate) * Utils::RandomFloat(0.5, 1.5));
}

// Move this to agent class?
float TrainingGround::EvaluateFitness(shared_ptr<Car> car_agent) {
    int closestBlockID = 0;

    float lowestDistanceSqr = FLT_MAX;
    // Get nearest track block to car
    for (auto &track_block :  training_track->track_blocks) {
        float distanceSqr = glm::length2(glm::distance(car_agent->car_body_model.position, track_block.center));
        if (distanceSqr < lowestDistanceSqr) {
            closestBlockID = track_block.block_id;
            lowestDistanceSqr = distanceSqr;
        }
    }

    // Return a number corresponding to the distance driven
    return (float) closestBlockID;
}

void TrainingGround::InitialiseAgents(uint16_t populationSize) {
    std::vector<CarModel> fce_models = NFS3::LoadFCE("./assets/car/NFS_3/diab/car.fce");

    // Create new cars, each with new RaceNetworks
    for (uint16_t pop_Idx = 0; pop_Idx < populationSize; ++pop_Idx) {
        shared_ptr<Car> car_agent = std::make_shared<Car>(pop_Idx, fce_models, NFS_3, "diab", RaceNet());
        physicsEngine.registerVehicle(car_agent);
        car_agent->resetCar(TrackUtils::pointToVec(this->training_track->track_blocks[0].center));
        car_agents.emplace_back(car_agent);
    }

    LOG(INFO) << "Agents initialised";
}

std::vector<std::vector<int>> TrainingGround::TrainAgents(uint16_t nGenerations, uint16_t nTicks) {
    std::vector<std::vector<int>> agentFitnesses;

    for (uint16_t gen_Idx = 0; gen_Idx < nGenerations; ++gen_Idx) {
        LOG(INFO) << "Beginning Generation " << gen_Idx;

        // Simulate the population
        for (uint16_t tick_Idx = 0; tick_Idx < nTicks; ++tick_Idx) {
            for (auto &car_agent : car_agents) {
                car_agent->simulate();
                if ((!(tick_Idx % 5000)) && (car_agent->populationID == 0)) {
                    LOG(INFO) << tick_Idx;
                }
            }
            physicsEngine.stepSimulation(stepTime);
        }

        // Clear fitness data for next generation
        agentFitnesses.clear();

        // Evaluate the fitnesses and sort them
        for (auto &car_agent : car_agents) {
            LOG(INFO) << "Agent " << car_agent->populationID << " made it to trkblock " << EvaluateFitness(car_agent);
            std::vector<int> agentData = {car_agent->populationID, (int) EvaluateFitness(car_agent)};
            agentFitnesses.emplace_back(agentData);
        }

        std::sort(agentFitnesses.begin(), agentFitnesses.end(),
                  [](const std::vector<int> &a, const std::vector<int> &b) {
                      return a[1] > b[1];
                  });


        LOG(DEBUG) << "Agent " << agentFitnesses[0][0] << " was fittest";

        // Mutate the fittest network
        Mutate(car_agents[agentFitnesses[0][0]]->carNet);

        // Reset the cars for the next generation
        for (auto &car_agent : car_agents) {
            car_agent->resetCar(TrackUtils::pointToVec(this->training_track->track_blocks[0].center));
        }
    }

    return agentFitnesses;
}
