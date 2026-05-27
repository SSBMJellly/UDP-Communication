#include <iostream>
#include <cmath>   //sqrt, acos, atan2, and M_PI
#include <vector>  //matrices/vectors or arrays

int main() {
    // Target cartesian coordinates
    double x = 13.0;
    double y = 3.0;
    double z = 3.0;

    // 0-indexed arrays replace MATLAB's 1-indexed vectors
    // MATLAB l(1) to l(5) becomes l[0] to l[4]
    double l[5] = { 10.0, 10.0, 12.5, 0.0, 17.5 };

    // Flattened 4x4 Transformation Matrix T05 (Row-major order)
    double T05[4][4] = {
        {0.0, 0.0, 1.0, x},
        {1.0, 0.0, 0.0, y},
        {0.0, 1.0, 0.0, z},
        {0.0, 0.0, 0.0, 1.0}
    };

    double q[5] = { 0.0 };

    // Manual Matrix-Vector multiplication replaces MATLAB's '*' operator
    double v1[4] = { 0.0, 0.0, 0.0, 1.0 };
    double v2[4] = { 0.0, 1.0, 0.0, 0.0 };
    double wrist[4] = { 0.0 };

    for (int i = 0; i < 4; ++i) {
        double T05_v1 = 0.0;
        double T05_v2 = 0.0;
        for (int j = 0; j < 4; ++j) {
            T05_v1 += T05[i][j] * v1[j];
            T05_v2 += T05[i][j] * v2[j];
        }
        wrist[i] = T05_v1 + (l[4] * T05_v2); // l(5) is l[4]
    }

    double x_wrist = wrist[0];
    double y_wrist = wrist[1];
    double z_wrist = wrist[2];

    //Math functions are called directly from std namespace
    double li_xcomp = std::sqrt(x_wrist * x_wrist + y_wrist * y_wrist);
    double li_ycomp = z_wrist - l[0]; // l(1) is l[0]
    double li = std::sqrt(li_xcomp * li_xcomp + li_ycomp * li_ycomp);

    double alpha1 = std::acos((li * li + l[2] * l[2] - l[1] * l[1]) / (2.0 * li * l[2])); // l(3)->l[2], l(2)->l[1]
    double beta1 = std::acos((li * li + l[1] * l[1] - l[2] * l[2]) / (2.0 * li * l[1]));
    double phi1 = std::atan2((z_wrist - l[0]), li_xcomp);

    //M_PI from <cmath> replaces MATLAB's 'pi'
    q[0] = std::atan2(y_wrist, x_wrist);
    q[1] = M_PI / 2.0 - (phi1 + beta1);
    q[2] = -(M_PI / 2.0 - (alpha1 + beta1));
    q[3] = -(q[1] + q[2]);
    q[4] = q[0];

    //std::cout replaces disp() for terminal output
    double rad_to_deg = 180.0 / M_PI;
    std::cout << "q1 = " << q[0] * rad_to_deg << "\n";
    std::cout << "q2 = " << q[1] * rad_to_deg << "\n";
    std::cout << "q3 = " << q[2] * rad_to_deg << "\n";
    std::cout << "q4 = " << q[3] * rad_to_deg << "\n";
    std::cout << "q5 = " << q[4] * rad_to_deg << "\n";

    return 0;
}
