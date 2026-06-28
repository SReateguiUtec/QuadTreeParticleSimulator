#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <cmath>
#include "httplib.h"
#include "Structure.h"
#include "Experiment.h"
#include "Visualization.h"
using namespace std;

struct SimState {
    mutex              mtx;
    vector<Particle>   particles;
    double             width    = 1000;
    double             height   = 1000;
    double             radius   = 4.0;
    int                capacity = 4;
    int                nextId   = 0;
} sim;


static double jsonDouble(const string& body, const string& key, double def = 0.0) {
    auto pos = body.find('"' + key + '"');
    if (pos == string::npos) return def;
    pos = body.find(':', pos);
    if (pos == string::npos) return def;
    try { return stod(body.substr(pos + 1)); } catch(...) { return def; }
}
static int jsonInt(const string& body, const string& key, int def = 0) {
    return (int)jsonDouble(body, key, (double)def);
}

// CORS helper
static void cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

int main() {
    httplib::Server svr;
    svr.set_read_timeout(3600);

    // ── OPTIONS (CORS)
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        cors(res);
        res.status = 204;
    });

    // ── GET /simulate/stream
    svr.Get("/simulate/stream", [](const httplib::Request& req, httplib::Response& res) {
        {
            lock_guard<mutex> lk(sim.mtx);
            if (req.has_param("w"))   sim.width    = stod(req.get_param_value("w"));
            if (req.has_param("h"))   sim.height   = stod(req.get_param_value("h"));
            if (req.has_param("r"))   sim.radius   = stod(req.get_param_value("r"));
            if (req.has_param("cap")) sim.capacity = stoi(req.get_param_value("cap"));
        }

        cout << "[stream] started  N=" << sim.particles.size() << "\n";

        cors(res);
        res.set_header("Cache-Control",     "no-cache");
        res.set_header("X-Accel-Buffering", "no");
        res.set_header("Connection",        "keep-alive");

        res.set_chunked_content_provider(
            "text/event-stream",
            [](size_t, httplib::DataSink& sink) -> bool {
                double w, h, r;
                int cap;
                {
                    lock_guard<mutex> lk(sim.mtx);
                    w = sim.width; h = sim.height;
                    r = sim.radius; cap = sim.capacity;
                }
                streamSimulationLive(
                    sim.mtx, sim.particles, w, h, r, cap,
                    [&sink](const string& json) -> bool {
                        string ev = "data: " + json + "\n\n";
                        return sink.write(ev.c_str(), ev.size());
                    }
                );
                sink.done();
                return true;
            }
        );
    });

    // ── POST /particle
    svr.Post("/particle", [](const httplib::Request& req, httplib::Response& res) {
        double x   = jsonDouble(req.body, "x");
        double y   = jsonDouble(req.body, "y");
        double spd = jsonDouble(req.body, "speed", 1.5);

        lock_guard<mutex> lk(sim.mtx);

        x = max(sim.radius, min(sim.width  - sim.radius, x));
        y = max(sim.radius, min(sim.height - sim.radius, y));

        double angle = (sim.nextId * 137.508) * M_PI / 180.0;
        Particle p;
        p.id     = sim.nextId++;
        p.x      = x;
        p.y      = y;
        p.vx     = spd * cos(angle);
        p.vy     = spd * sin(angle);
        p.radius = sim.radius;
        sim.particles.push_back(p);

        cors(res);
        res.set_content(
            "{\"ok\":true,\"id\":" + to_string(p.id) + ",\"count\":" + to_string(sim.particles.size()) + "}",
            "application/json"
        );
        cout << "[particle] added id=" << p.id << " at (" << x << "," << y << ")  total=" << sim.particles.size() << "\n";
    });

    // DELETE /particles
    svr.Delete("/particles", [](const httplib::Request&, httplib::Response& res) {
        lock_guard<mutex> lk(sim.mtx);
        sim.particles.clear();
        sim.nextId = 0;
        cors(res);
        res.set_content("{\"ok\":true,\"count\":0}", "application/json");
        cout << "[particles] cleared\n";
    });

    // POST /particles/generate
    svr.Post("/particles/generate", [](const httplib::Request& req, httplib::Response& res) {
        int    n    = jsonInt   (req.body, "n",    200);
        int    dist = jsonInt   (req.body, "dist",   0);
        double w, h, r;
        {
            lock_guard<mutex> lk(sim.mtx);
            w = sim.width; h = sim.height; r = sim.radius;
        }

        vector<Particle> generated;
        DistributionType dtype = (dist >= 0 && dist <= 2)
            ? static_cast<DistributionType>(dist)
            : DistributionType::Uniform;
        generated = generateParticles(dtype, n, w, h, r, rand());

        {
            lock_guard<mutex> lk(sim.mtx);
            sim.particles.clear();
            sim.nextId = 0;
            for (auto& p : generated) { p.id = sim.nextId++; }
            sim.particles = generated;
        }

        cors(res);
        res.set_content(
            "{\"ok\":true,\"count\":" + to_string(generated.size()) + "}",
            "application/json"
        );
        cout << "[generate] dist=" << dist << " n=" << generated.size() << "\n";
    });

    // GET /experiment
    svr.Get("/experiment", [](const httplib::Request& req, httplib::Response& res) {
        double w   = req.has_param("w")   ? stod(req.get_param_value("w"))   : 1000;
        double h   = req.has_param("h")   ? stod(req.get_param_value("h"))   : 1000;
        double r   = req.has_param("r")   ? stod(req.get_param_value("r"))   : 4.0;
        int    cap = req.has_param("cap") ? stoi(req.get_param_value("cap")) : 4;
        int frames = req.has_param("frames") ? stoi(req.get_param_value("frames")) : 30;

        vector<int> sizes = {1000, 5000, 10000};
        vector<DistributionType> dists = {
            DistributionType::Uniform,
            DistributionType::Clusters,
            DistributionType::HighDensity
        };

        cors(res);
        res.set_header("Cache-Control", "no-cache");

        string json = "[";
        bool first = true;
        for (int n : sizes) {
            for (auto dist : dists) {
                cout << "[experiment] running n=" << n << " dist=" << distributionName(dist) << "\n";
                ExperimentResult er = runExperiment(dist, n, frames, w, h, r, cap);

                if (!first) json += ",";
                first = false;

                double speedup = er.averageBruteForceTimeUs > 0
                    ? er.averageBruteForceTimeUs / max(er.averageQuadTreeTimeUs, 1.0)
                    : 0.0;

                json += "{";
                json += "\"distribution\":\"" + er.distributionName + "\",";
                json += "\"n\":" + to_string(er.particleCount) + ",";
                json += "\"frames\":" + to_string(er.frames) + ",";
                json += "\"buildTimeUs\":" + to_string((long long)er.averageBuildTimeUs) + ",";
                json += "\"qtTimeUs\":" + to_string((long long)er.averageQuadTreeTimeUs) + ",";
                json += "\"bfTimeUs\":" + to_string((long long)er.averageBruteForceTimeUs) + ",";
                json += "\"qtComparisons\":" + to_string((long long)er.averageQuadTreeComparisons) + ",";
                json += "\"bfComparisons\":" + to_string((long long)er.averageBruteForceComparisons) + ",";
                json += "\"nodesVisited\":" + to_string((long long)er.averageQuadTreeNodesVisited) + ",";
                json += "\"particlesChecked\":" + to_string((long long)er.averageQuadTreeParticlesChecked) + ",";
                json += "\"candidatesFound\":" + to_string((long long)er.averageQuadTreeCandidatesFound) + ",";
                json += "\"collisions\":" + to_string((long long)er.averageCollisions) + ",";
                json += "\"candidatesPerParticle\":" + to_string(er.averageCandidatesPerParticle) + ",";
                json += "\"speedup\":" + to_string(speedup);
                json += "}";
            }
        }
        json += "]";

        res.set_content(json, "application/json");
        cout << "[experiment] done\n";
    });

    // GET /simulate
    svr.Get("/simulate", [](const httplib::Request& req, httplib::Response& res) {
        double w  = req.has_param("w")   ? stod(req.get_param_value("w"))   : 1000;
        double h  = req.has_param("h")   ? stod(req.get_param_value("h"))   : 1000;
        double r  = req.has_param("r")   ? stod(req.get_param_value("r"))   : 3.0;
        int cap   = req.has_param("cap") ? stoi(req.get_param_value("cap")) : 4;
        int n     = req.has_param("n")   ? stoi(req.get_param_value("n"))   : 420;
        int dist  = req.has_param("dist")? stoi(req.get_param_value("dist")): 3;
        string json = generateVisualizationFramesJson(w, h, r, cap, n, 160, dist);
        cors(res);
        res.set_content(json, "application/json");
    });

    cout << "==========================================\n";
    cout << " QuadTree Backend \n";
    cout << "👉 http://localhost:8080                   \n";
    cout << "   GET  /simulate/stream     → SSE live   \n";
    cout << "   POST /particle            → add 1 pt   \n";
    cout << "   DELETE /particles         → clear all  \n";
    cout << "   POST /particles/generate  → bulk gen   \n";
    cout << "==========================================\n";

    svr.listen("0.0.0.0", 8080);
    return 0;
}
