//
// Created by Amrik on 28/10/2018.
//

#pragma once

#include <vector>
#include "stdint.h"

#include "../Loaders/trk_loader.h"
#include "../Loaders/nfs3_loader.h"
#include "../Physics/Physics.h"
#include "../Util/Utils.h"

static const float stepTime = 1 / 60.f;

class TrainingGround {
public:
    explicit TrainingGround(uint16_t populationSize, uint16_t nGenerations, uint16_t nTicks, shared_ptr<ONFSTrack> training_track);
    float EvaluateFitness(shared_ptr<Car> car_agent);
private:
    void InitialiseAgents(uint16_t populationSize);
    std::vector<std::vector<int>> TrainAgents(uint16_t nGenerations, uint16_t nTicks); // Train the agents, returning agent fitness data
    void Mutate(RaceNet &toMutate);
    shared_ptr<ONFSTrack> training_track;
    std::vector<shared_ptr<Car>> car_agents;
    /*------- BULLET --------*/
    Physics physicsEngine;
};
