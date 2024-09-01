#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <iostream>

// To avoid doing "Eigen::Matrix...".
using namespace Eigen;

int main()
{
	// Dynamic - resizable.
	MatrixXd d;

	// Fixed Size.
	Matrix3d f;

	std::cout << f.size() << std::endl; // 9

	f << 1, 2, 3,
		4, 5, 6,
		7, 8, 9;

	std::cout << f << std::endl;

	d = MatrixXd::Random(5, 5);

	std::cout << d << std::endl;

	std::cin.get();
}