#include <Eigen/Core>
#include <Eigen/Dense>
#include <vector>
#include <strstream>
#include <fstream>
#include <set>
#include <map>
using namespace Eigen;

MatrixXd V;
MatrixXi F;
std::vector<std::vector<int> > IrregularF;

void Upsampling()
{
	std::map<std::pair<int, int>, int> edgeid;
	for (int i = 0; i < F.cols(); ++i) {
		for (int j = 0; j < 4; ++j) {
			int v1 = F(j, i);
			int v2 = F((j + 1) % 4, i);
			if (v1 > v2)
				std::swap(v1, v2);
			auto key = std::make_pair(v1, v2);
			int s = edgeid.size();
			if (edgeid.count(key) == 0)
				edgeid[key] = s;
		}
	}
	std::vector<Vector4i> faces(F.cols() * 4);
	std::vector<Vector3d> vertices(V.cols() + edgeid.size() + F.cols());
	for (int i = 0; i < V.cols(); ++i) {
		vertices[i] = V.col(i);
	}
	for (auto info : edgeid)
		vertices[V.cols() + info.second] = 0.5 * (V.col(info.first.first) + V.col(info.first.second));
	for (int i = 0; i < F.cols(); ++i) {
		Vector3d p(0, 0, 0);
		for (int j = 0; j < 4; ++j) {
			p += V.col(F(j, i));
		}
		p *= 0.25;
		vertices[V.cols() + edgeid.size() + i] = p;
	}
	for (int i = 0; i < F.cols(); ++i) {
		int eid[4];
		for (int j = 0; j < 4; ++j) {
			int v1 = F(j, i);
			int v2 = F((j + 1) % 4, i);
			if (v1 > v2)
				std::swap(v1, v2);
			auto key = std::make_pair(v1, v2);
			if (edgeid.count(key) == 0) {
				printf("OMG!\n");
			}
			eid[j] = edgeid[key];
		}
		for (int j = 0; j < 4; ++j) {
			faces[i * 4 + j] =
				Vector4i(F(j, i), eid[j] + V.cols(), V.cols() + edgeid.size() + i, eid[(j + 3) % 4] + V.cols());
		}
	}
	V.resize(3, vertices.size());
	memcpy(V.data(), vertices.data(), sizeof(double) * 3 * vertices.size());
	F.resize(4, faces.size());
	memcpy(F.data(), faces.data(), sizeof(int) * 4 * faces.size());
}
void Load(const char* filename) {
	std::vector<Vector3d> positions;
	std::vector<Vector4i> faces;
	std::ifstream is(filename);
	char buffer[2048];
	while (is.getline(buffer, 2048)) {
		std::strstream str;
		str << buffer;
		str >> buffer;
		if (strcmp(buffer, "v") == 0) {
			double x, y, z;
			str >> x >> y >> z;
			positions.push_back(Vector3d(x, y, z));
		}
		else if (strcmp(buffer, "f") == 0) {
			std::vector<int> face;
			while (str >> buffer) {
				std::strstream fstr;
				fstr << buffer;
				int f;
				fstr >> f;
				face.push_back(f-1);
			}
			if (face.size() == 4) {
				Vector4i f(face[0], face[1], face[2], face[3]);
				faces.push_back(f);
			} else {
				IrregularF.push_back(face);
			}
		}
	}
	V.resize(3, positions.size());
	memcpy(V.data(), positions.data(), sizeof(double) * 3 * positions.size());
	F.resize(4, faces.size());
	memcpy(F.data(), faces.data(), sizeof(int) * 4 * faces.size());
}

void ReportFaceInfo()
{
	printf("Vertex: %d      Regular Faes: %d       Irregular faces %d\n", V.cols(), F.cols(), IrregularF.size());
}

void ReportManifold()
{
	std::set<std::pair<int, int> > dedges;
	bool isManifold = true;
	for (int i = 0; i < F.cols(); ++i) {
		for (int j = 0; j < 4; ++j) {
			int v1 = F(j, i);
			int v2 = F((j + 1) % 4, i);
			auto key = std::make_pair(v1, v2);
			if (dedges.count(key)) {
				isManifold = false;
			} else {
				dedges.insert(key);
			}
		}
	}
	for (int i = 0; i < IrregularF.size(); ++i) {
		for (int j = 0; j < IrregularF[i].size(); ++j) {
			int v1 = IrregularF[i][j];
			int v2 = IrregularF[i][(j + 1) % IrregularF[i].size()];
			auto key = std::make_pair(v1, v2);
			if (dedges.count(key)) {
				isManifold = false;
			} else {
				dedges.insert(key);
			}			
		}
	}
	if (isManifold) {
		printf("Is Manifold: True.\n");
	} else {
		printf("Is Manifold: False.\n");
	}
}

void ReportSingularity()
{
	std::vector<std::set<int> > links(V.cols());
	for (int i = 0; i < F.cols(); ++i) {
		for (int j = 0; j < 4; ++j) {
			int v1 = F(j, i);
			int v2 = F((j + 1) % 4, i);
			links[v1].insert(v2);
			links[v2].insert(v1);
		}
	}
	for (int i = 0; i < IrregularF.size(); ++i) {
		for (int j = 0; j < IrregularF[i].size(); ++j) {
			int v1 = IrregularF[i][j];
			int v2 = IrregularF[i][(j + 1) % IrregularF[i].size()];
			links[v1].insert(v2);
			links[v2].insert(v1);
		}
	}
	std::map<int, int> valences;
	for (int i = 0; i < links.size(); ++i) {
		int v = links[i].size();
		if (v != 0) {
			if (valences.count(v)) {
				valences[v] += 1;
			} else {
				valences[v] = 1;
			}
		}
	}
	for (auto& p : valences) {
		printf("Valence %d: %d\n", p.first, p.second);
	}
}

void ReportAngleDifference()
{
	double e = 0, max_angle = 0, min_angle = 360;
	for (int i = 0; i < F.cols(); ++i) {
		for (int j = 0; j < 4; ++j) {
			int v0 = F(j, i);
			int v1 = F((j+1)%4,i);
			int v2 = F((j+3)%4,i);
			Vector3d d1 = V.col(v1) - V.col(v0);
			Vector3d d2 = V.col(v2) - V.col(v0);
			d1.normalize();
			d2.normalize();
			double angle = 180.0/3.141592654*atan2(d1.cross(d2).norm(), d1.dot(d2));
			e += (angle-90)*(angle-90);
			max_angle = std::max(angle, max_angle);
			min_angle = std::min(angle, min_angle);
		}
	}
	printf("min max angle: %lf %lf\n", min_angle, max_angle);
	printf("angle average error: %lf\n", sqrt(e / 4 / F.cols()));
}

void Analyze()
{
	ReportFaceInfo();
	ReportManifold();
	ReportSingularity();
	ReportAngleDifference();
}

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("./analyzer input.obj [scale] [output.txt]\n");
		return 0;
	}

	Load(argv[1]);

	int scale = 1;
	if (argc >= 3) {
		sscanf(argv[2], "%d", &scale);
	}
	for (int i = 1; i < scale; ++i) {
		Upsampling();
	}
	if (argc >= 4) {
		freopen(argv[3],"w",stdout);
	}

	Analyze();

	fclose(stdout);
}
