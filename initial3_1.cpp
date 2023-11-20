#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <unordered_map>
#include <queue>
#include <limits>

using namespace std;

// Node Types
enum NodeType
{
    SPACE_NODE,
    AIR_NODE,
    GROUND_NODE
};

// Resources
struct Resources
{
    int cpu;
    int availableCpu;
};

// Node
struct Node
{
    int id;
    NodeType nodeType;
    Resources resources;
};

// Link
struct LinkInfo
{
    int node1;
    int node2;
    int bandwidth;
    int availableBandwidth;
    int delay;
};

struct VNF
{
    int vnfId;
    int cpuReq;
    int vnfDelay;
};

struct SFC
{
    int sfcId;
    vector<VNF> vnfs;
    int maxDelay;
};

// initialize nodes and links from a file
void initializeNodesAndLinks(vector<Node> &nodes, vector<LinkInfo> &links)
{
    ifstream inputFile("input.txt");

    if (!inputFile.is_open())
    {
        cerr << "Error: Unable to open file " << endl;
        exit(EXIT_FAILURE);
    }

    int numNodes;
    inputFile >> numNodes;

    for (int i = 0; i < numNodes; ++i)
    {
        Node node;
        inputFile >> node.id >> reinterpret_cast<int &>(node.nodeType) >> node.resources.cpu;
        node.resources.availableCpu = node.resources.cpu;
        nodes.push_back(node);
    }

    int numLinks;
    inputFile >> numLinks;

    for (int i = 0; i < numLinks; ++i)
    {
        LinkInfo link;
        inputFile >> link.node1 >> link.node2 >> link.bandwidth >> link.delay;
        link.availableBandwidth = link.bandwidth;

        links.push_back(link);
        swap(link.node1, link.node2);
        links.push_back(link);
    }

    inputFile.close();
}

// initialize VNFs with random CPU requirements
vector<VNF> initializeVNFs()
{
    vector<VNF> vnfs;
    ifstream inputFile("vnfs.txt");

    if (!inputFile.is_open())
    {
        cerr << "Error: Unable to open file " << endl;
        exit(EXIT_FAILURE);
    }

    int numVNFs;
    inputFile >> numVNFs;

    for (int i = 1; i <= numVNFs; ++i)
    {
        VNF vnf;
        inputFile >> vnf.vnfId >> vnf.cpuReq >> vnf.vnfDelay;
        vnfs.push_back(vnf);
    }

    inputFile.close();
    return vnfs;
}

// initialize SFCs with random sequences of VNFs and max delay
vector<SFC> initializeSFCs(const vector<VNF> &allVNFs)
{
    vector<SFC> sfcs;
    ifstream inputFile("sfcs.txt");

    if (!inputFile.is_open())
    {
        cerr << "Error: Unable to open file " << endl;
        exit(EXIT_FAILURE);
    }

    int numSFCs;
    inputFile >> numSFCs;

    for (int i = 1; i <= numSFCs; ++i)
    {
        SFC sfc;
        sfc.sfcId = i;

        int numVNFsInSFC;
        inputFile >> numVNFsInSFC;

        for (int j = 0; j < numVNFsInSFC; ++j)
        {
            int vnfId;
            inputFile >> vnfId;

            auto it = find_if(allVNFs.begin(), allVNFs.end(), [vnfId](const VNF &vnf)
                              { return vnf.vnfId == vnfId; });

            if (it != allVNFs.end())
            {
                sfc.vnfs.push_back(*it);
            }
            else
            {
                cerr << "Error: VNF with ID " << vnfId << " not found." << endl;
                exit(EXIT_FAILURE);
            }
        }
        int tempDelay;
        inputFile >> tempDelay;
        sfc.maxDelay = tempDelay;

        sfcs.push_back(sfc);
    }

    inputFile.close();
    return sfcs;
}

// sorting SFCs in descending order of max delay
bool compareSFCs(const SFC &sfc1, const SFC &sfc2)
{
    return sfc1.maxDelay > sfc2.maxDelay;
}

// show all the SFCs and their associated VNFs
void showSFCs(const vector<SFC> &sfcs)
{
    cout << "Service Function Chains (SFCs):" << endl;
    for (const SFC &sfc : sfcs)
    {
        cout << "SFC ID: " << sfc.sfcId << ", Max Delay: " << sfc.maxDelay << endl;
        cout << "VNFs:" << endl;
        for (const VNF &vnf : sfc.vnfs)
        {
            cout << "  VNF ID: " << vnf.vnfId << ", CPU Requirement: " << vnf.cpuReq << ", VNF Delay: " << vnf.vnfDelay << endl;
        }
        cout << "------------------------" << endl;
    }
}

// show the network topology
void showTopology(const vector<Node> &nodes, const vector<LinkInfo> &links)
{
    cout << "Topology:" << endl;

    // Print nodes
    cout << "Nodes:" << endl;
    for (const Node &node : nodes)
    {
        cout << "Node " << node.id << " (Type: ";
        switch (node.nodeType)
        {
        case SPACE_NODE:
            cout << "Space";
            break;
        case AIR_NODE:
            cout << "Air";
            break;
        case GROUND_NODE:
            cout << "Ground";
            break;
        }
        cout << ", CPU: " << node.resources.cpu << ")" << endl;
    }

    // Print links
    cout << "Links:" << endl;
    for (const LinkInfo &link : links)
    {
        cout << "Link between Node " << link.node1 << " and Node " << link.node2
             << " (Bandwidth: " << link.bandwidth << ", Delay: " << link.delay << ")" << endl;
    }
}

// Function to find shortest path of length 4 nodes with least link delay
vector<int> findShortestPath(const vector<Node> &nodes, const vector<LinkInfo> &links, const SFC &sfc)
{
    int startNode = -1;
    int maxAvailableCPU = -1;

    // Find the node with the highest available CPU
    for (const Node &node : nodes)
    {
        if (node.resources.availableCpu > maxAvailableCPU)
        {
            maxAvailableCPU = node.resources.availableCpu;
            startNode = node.id;
        }
    }

    // Dijkstra's algorithm to find the shortest path
    unordered_map<int, vector<pair<int, int>>> graph;
    for (const LinkInfo &link : links)
    {
        graph[link.node1].push_back({link.node2, link.delay});
        graph[link.node2].push_back({link.node1, link.delay});
    }

    priority_queue<pair<int, vector<int>>, vector<pair<int, vector<int>>>, greater<pair<int, vector<int>>>> pq;
    pq.push({0, {startNode}});
    unordered_map<int, int> dist;
    unordered_map<int, vector<int>> shortestPaths;

    while (!pq.empty())
    {
        int cost = pq.top().first;
        vector<int> path = pq.top().second;
        int currentNode = path.back();
        pq.pop();

        if (path.size() == 5)
        { // Found a path of length 4 nodes
            shortestPaths[cost] = path;
            continue;
        }

        for (const auto &neighbor : graph[currentNode])
        {
            int neighborNode = neighbor.first;
            int weight = neighbor.second;
            if (find(path.begin(), path.end(), neighborNode) == path.end())
            {
                vector<int> newPath = path;
                newPath.push_back(neighborNode);
                pq.push({cost + weight, newPath});
            }
        }
    }

    if (shortestPaths.empty())
    {
        cerr << "No path of length 4 nodes found." << endl;
        return {};
    }

    // Return the shortest path with the least delay
    return shortestPaths.begin()->second;
}

int main()
{
    // string filename = "input.txt";

    vector<Node> nodes;
    vector<LinkInfo> links;
    initializeNodesAndLinks(nodes, links);

    showTopology(nodes, links);

    vector<VNF> allVNFs = initializeVNFs();

    vector<SFC> allSFCs = initializeSFCs(allVNFs);

    showSFCs(allSFCs);

    // showAllArrangements(nodes, allSFCs[0]);

    sort(allSFCs.begin(), allSFCs.end(), compareSFCs);

    cout << endl;
    cout << "SFCs have been sorted" << endl;
    cout << endl;

    showSFCs(allSFCs);

    // Iterate through each SFC to find and print shortest paths
    for (const SFC &sfc : allSFCs)
    {
        cout << "Shortest Path for SFC ID " << sfc.sfcId << ":" << endl;
        vector<int> shortestPath = findShortestPath(nodes, links, sfc);

        if (!shortestPath.empty())
        {
            cout << "Path: ";
            for (int node : shortestPath)
            {
                cout << node << " -> ";
            }
            cout << "End" << endl;
        }
    }

    return 0;
}
