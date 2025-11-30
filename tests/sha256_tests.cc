#include "tests.h"
#include "utils/sha256.h"

#include <iostream>
#include <string>
#include <string_view>

bool test_helper(std::string_view expected, std::string_view result) {
    if (result == expected) {
        std::cout << "[PASS]" << std::endl;
        return true;
    } else {
        std::cout << "[FAIL] Expected: " << expected << std::endl;
        std::cout << "[FAIL] Got:      " << result << std::endl;
        return false;
    }
}

bool Sha256TestEmptyString() {
    std::cout << "[TEST] Sha256: Empty string" << std::endl;

    std::string input = "";
    std::string expected = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    std::string result = core::utils::Sha256::Compute(input);

    return test_helper(expected, result);
}

bool Sha256TestNormalString() {
    std::cout << "[TEST] Sha256: \"Hello World!\"" << std::endl;

    std::string input = "Hello World!";
    std::string expected = "7f83b1657ff1fc53b92dc18148a1d65dfc2d4b1fa3d677284addd200126d9069";
    std::string result = core::utils::Sha256::Compute(input);

    return test_helper(expected, result);
}

bool Sha256TestNonAscii() {
    std::cout << "[TEST] Sha256: Non-ascii characters" << std::endl;

    std::string input = "Δ♥ж∞字か한😁";
    std::string expected = "7489042d3319b1c59494218f632d021a663f40ca75a5293558316dab3b25dbf7";
    std::string result = core::utils::Sha256::Compute(input);

    return test_helper(expected, result);
}

bool Sha256TestLargeString512Chars() {
    std::cout << "[TEST] Sha256: 512 characters" << std::endl;

    std::string input = "$V!]UM0k1m{pxtNb,Z)PeF&D+%=52cAv;d{hi!xxmFf86F({E=w+tB:Uyc&f1rpCz3D/f&;4i{GhZQE;3@4?2:?SKTAZ([+jJ17wPa3!+G=2.Hw90n]uxZX+gH3=Ug3bNL9{Ld)h{+B&yMC_F&NT@WuJJ}R7(ZJ17XV{R8!.4fLe#*MDi_Y#Ecrx,_F4i5}P_9MY+Cdd[$2vQFALhWRg[YCaA/d6mXV(JYeYR%Ai1xp@ArSybJmWqRG${HM!vXcGFuZVJ]n4Ux%1T#Qx4.kD!qkk-J+AA78%_07D+HvDx1G3.$BX_nDMgfMN:SZMSnkj/14zvVku7e:.%(hCaap=82%cFnR!dwuF/cBZcRW6eg&y,}E6By6=P9kh)HC&Q59EdS5wv*nFH_:hhuPUByf3ua&[_$}}K[i}Xiwu-_3U21p+0Sj1Vf#K8ea7,uE4f7h?-u.X+-+b_@xALS)f1j,@C{J.FaVQXB%z){&rJFpuHVe:]_pVXDM8u)c.XmD%Nz4{";
    std::string expected = "736f6ad52f068843776af3fa2462c1f66d3450da1361e039dd6cdecc7b9e3e6d";
    std::string result = core::utils::Sha256::Compute(input);

    return test_helper(expected, result);
}

bool Sha256TestLargeString513Chars() {
    std::cout << "[TEST] Sha256: 513 characters" << std::endl;

    std::string input = "-.g/X@SCN!Nd*by2wSpg=)hU/YvFmGbXd]0_V0UnaK2?fHLFa)T3*B8*0PF_/+y=:An5W2SeH9SjrfzxZ/fVY8Px)&E.JrV,5Xd#!yuaf=D2CGK+*yJ:qQ;,tHfj?nWD@#4gJ&1f@R+RzK576cFWLL{CaT_aJS4cz45zu*N!*9rS7yn*{S]w3hce0MuH/xaXpQ&t27xy,F:0fd[!+VkguRQuRw;be;J2C{G(4!p%]k,DiDb09qg_&7pdt_:u:WqBgt/ijiC]GUMaGK05{CH&x-X:5=yZr#)8ik?NvkVJ-L=yGg;77?ZaZ-=$895_d*NU8=!&1r&qJK!.q1HL(.Gdd1Vq1wUDcQ5z}UcP4@B0*E-y3bN#Mj!+=3BN/UR{n?#*mPq6+#=h[v0/kptZ]zcQzmf6ci7k&y=JFf/{*}3K]%@P-_0[2_apm:YJgJ#WP$vHq8_6pZ+5dzCt8@puhkuV{]Lbm%%xCN$mHD=wa]EVv#WPui7GNJi8YC-aU=c%Jbx1&";
    std::string expected = "ddca05c5a1c7d1e214eb000a49762ec77de10ee6ba13b3d4a56ec131048898c3";
    std::string result = core::utils::Sha256::Compute(input);

    return test_helper(expected, result);
}

bool Sha256TestSingleCharacter() {
    std::cout << "[TEST] Sha256: Single character (a)" << std::endl;

    std::string input = "a";
    std::string expected = "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb";
    std::string result = core::utils::Sha256::Compute(input);

    return test_helper(expected, result);
}

bool Sha256TestPaddingBoundary55() {
    //Fits in one block
    //55 + 1 (0x80) + 8 = 64 (one block)
    std::cout << "[TEST] Sha256: 55 characters --> one block" << std::endl;

    std::string input = "3)Z+DFByp.XcNV&97mFc?(UnGk-%{gCtaiX}_]![gqpt/@:gL]/7!zi";
    std::string expected = "b5cc35a6d98b224e6d3b2407b8f40d565f64a5c5c752c82a4dbc66be7e0b6136";
    std::string result = core::utils::Sha256::Compute(input);

    return test_helper(expected, result);
}

bool Sha256TestPaddingBoundary56() {
    //Fits in two blocks
    //56 + 1 (0x80) + 8 = 65 (two blocks)
    std::cout << "[TEST] Sha256: 56 characters --> two blocks" << std::endl;

    std::string input = "M{,2FNNR}Lw+Y/D7%]!TbXQe9:JvEe#cg)v@UW!1HZSMSw%%9$/8e4$8";
    std::string expected = "4cfc59ebeed3ac3d4111fc64a9bfb5402ae91ddbcd14be2cd21164d294e9caee";
    std::string result = core::utils::Sha256::Compute(input);

    return test_helper(expected, result);
}

bool Sha256TestRepeatedPattern() {
    std::cout << "[TEST] Sha256: Repeated pattern of abc" << std::endl;

    std::string input = "abcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabc";
    std::string expected = "7c8c7ba2f9483c5a7960c020efe63de3538f7f3d30be316626aa349d0d0f5c11";
    std::string result = core::utils::Sha256::Compute(input);

    return test_helper(expected, result);
}

bool Sha256TestBinaryData() {
    std::cout << "[TEST] Sha256: Binary data" << std::endl;

    const char* input_data = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F";
    std::string_view input(input_data, 16); //Special handling because the binary data starts with a null terminator
    std::string expected = "be45cb2605bf36bebde684841a28f0fd43c69850a3dce5fedba69928ee3a8991";
    std::string result = core::utils::Sha256::Compute(input);

    return test_helper(expected, result);
}

bool Sha256TestNewlineCharacters() {
    std::cout << "[TEST] Sha256: Newline characters" << std::endl;

    std::string input = "\n\n\n";
    std::string expected = "6a3cf5192354f71615ac51034b3e97c20eda99643fcaf5bbe6d41ad59bd12167";
    std::string result = core::utils::Sha256::Compute(input);

    return test_helper(expected, result);
}