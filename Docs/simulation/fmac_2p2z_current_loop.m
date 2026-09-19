%% FMAC 2P2Z current-loop design and simulation
% Follows ST AN5788 and RM0440:
% y[n] = A1*y[n-1] + A2*y[n-2] + B0*x[n] + B1*x[n-1] + B2*x[n-2]
% FMAC coefficient order is [B0 B1 B2 A1 A2].
% Replace power-stage and sensing parameters below with measured values.

clear; close all; clc;

%% User parameters
cfg.Fs = 50e3;
cfg.Ts = 1/cfg.Fs;
cfg.PwmFs = 200e3;             % HRTIM switching frequency from BSP_CONFIG.md
cfg.Vbus = 48;                 % V, nominal input/output voltage
cfg.L = 15e-6;                 % H, PQ2012
cfg.RL = 6.5e-3;               % ohm, PQ2012 winding resistance
cfg.Cin = 470e-6;              % F, BSP_CONFIG.md (not dominant in inner-current model)
cfg.Cout = 470e-6;             % F, BSP_CONFIG.md (not dominant in inner-current model)
cfg.ESRin = 25e-3;             % ohm, BSP_CONFIG.md
cfg.ESRout = 25e-3;            % ohm, BSP_CONFIG.md
cfg.delaySamples = 1.0;
cfg.Fcross = 1.5e3;            % Hz, conservative first design below Fs/20
cfg.fz1 = 70;                 % Hz, initial placement; tune with measured plant
cfg.fz2 = 500;                % Hz, initial placement; tune with measured plant
cfg.fp  = 8e3;                % Hz, high-frequency pole
cfg.errorFullScale_A = 11;    % A, hardware OCP peak current
cfg.dutyFullScale = 1.0;
cfg.adcBits = 12;
cfg.adcLeftAligned = false;
cfg.vref = 3.3;               % ADC reference [V]
cfg.currentSenseR = 8e-3;     % current shunt [ohm]
cfg.currentSenseGain = 20;    % INA240A1 gain [V/V]
cfg.currentOffsetV = 1.65;    % measured by VREF165, not hard-coded in firmware
cfg.extraInputSlots = 1;      % RM0440 d1
cfg.extraOutputSlots = 1;     % RM0440 d2

%% Continuous plant and Type-II compensator
s = tf('s');
% First-order inductor-current plant for one active buck/boost leg. The
% four-switch buck-boost has different gains in buck and boost directions;
% replace plantVoltage with the measured small-signal value for each mode.
cfg.plantVoltage = cfg.Vbus;
Gplant_s = cfg.plantVoltage/(cfg.L*s + cfg.RL);
wz1 = 2*pi*cfg.fz1;
wz2 = 2*pi*cfg.fz2;
wp  = 2*pi*cfg.fp;
wcx = 2*pi*cfg.Fcross;
Cshape_s = (1+s/wz1)*(1+s/wz2)/(s*(1+s/wp));
delay_s = pade(tf(1), cfg.delaySamples*cfg.Ts, 1);
Kc = 1/abs(squeeze(freqresp(Cshape_s*Gplant_s*delay_s, wcx)));
% Normalize physical error/current to FMAC input and duty to FMAC output.
Knorm = Kc*cfg.errorFullScale_A/cfg.dutyFullScale;
C_s = Knorm*Cshape_s;

%% Tustin discretization and FMAC coefficient extraction
C_z = c2d(C_s, cfg.Ts, 'tustin');
[numC, denC] = tfdata(C_z, 'v');
numC = numC/denC(1);
denC = denC/denC(1);
numC = [zeros(1, max(0,3-numel(numC))) numC];
denC = [zeros(1, max(0,3-numel(denC))) denC];
B = numC(end-2:end);          % [B0 B1 B2]
A_std = denC(end-1:end);      % denominator [1 A1 A2]
% MATLAB uses 1+A1*z^-1+A2*z^-2; FMAC adds feedback terms.
A_fmac = -A_std;
z = tf('z', cfg.Ts);
C_fmac_z = (B(1)+B(2)*z^-1+B(3)*z^-2) / ...
           (1-A_fmac(1)*z^-1-A_fmac(2)*z^-2);

%% FMAC Q1.15 conversion
% FMAC computes 2^R*(B*X+A*Y). Scale all coefficients together.
coef = [B A_fmac];
R = max(0, ceil(log2(max(abs(coef))/0.999969)));
R = min(R, 7);
coefScaled = coef/(2^R);
q15 = round(max(-1, min(0.999969, coefScaled))*2^15);
q15_u16 = mod(q15, 2^16);
preShift = 0;
postShift = R;
if cfg.adcLeftAligned
    preShift = 0;              % caller supplies signed 16-bit ADC-domain data
end

%% RM0440 IIR local-memory layout
N = 3; M = 2;
mem.X2_BASE = 0;
mem.X2_SIZE = N + M;
mem.X1_BASE = mem.X2_BASE + mem.X2_SIZE;
mem.X1_SIZE = N + cfg.extraInputSlots;
mem.Y_BASE = mem.X1_BASE + mem.X1_SIZE;
mem.Y_SIZE = M + cfg.extraOutputSlots;
assert(mem.Y_BASE + mem.Y_SIZE <= 256, 'FMAC local memory exceeds 256 words');

%% Closed-loop simulation
% Convert plant output from amperes to normalized current. This must match
% x_norm=e_A/errorFullScale_A and y_norm=duty/dutyFullScale above.
Gplant_norm_s = Gplant_s*cfg.dutyFullScale/cfg.errorFullScale_A;
Gplant_z = c2d(Gplant_norm_s, cfg.Ts, 'zoh');
delay_z = z^(-cfg.delaySamples);
Tclosed = feedback(C_fmac_z*Gplant_z*delay_z, 1);
t = (0:round(0.02/cfg.Ts))*cfg.Ts;
[yStep, tStep] = step(Tclosed, t);
Loop_z = C_fmac_z*Gplant_z*delay_z;
[Gm, Pm, Wcg, Wcp] = margin(Loop_z);

%% Console output and firmware export
fprintf('\nFMAC 2P2Z current-loop design\n');
fprintf('Fs=%.1f Hz, Fcross=%.1f Hz, Kc=%.8g\n', cfg.Fs, cfg.Fcross, Kc);
fprintf('PWM/control ratio = %.3g, current ADC scale = %.6g V/A\n', cfg.PwmFs/cfg.Fs, cfg.currentSenseR*cfg.currentSenseGain);
adcCountsPerAmp = cfg.currentSenseR*cfg.currentSenseGain/cfg.vref*(2^cfg.adcBits-1);
fprintf('ADC current scale = %.6g counts/A, OCP delta at %.3g A = %.6g counts\n', adcCountsPerAmp, cfg.errorFullScale_A, adcCountsPerAmp*cfg.errorFullScale_A);
fprintf('B=[%.10g %.10g %.10g]\n', B);
fprintf('A_fmac=[%.10g %.10g]\n', A_fmac);
fprintf('R=%d, pre_shift=%d, post_shift=%d\n', R, preShift, postShift);
fprintf('Q15 B=[0x%04X 0x%04X 0x%04X]\n', q15_u16(1), q15_u16(2), q15_u16(3));
fprintf('Q15 A=[0x%04X 0x%04X]\n', q15_u16(4), q15_u16(5));
fprintf('PM=%.2f deg, GM=%.2f dB\n', Pm, 20*log10(Gm));

outDir = fileparts(mfilename('fullpath'));
outFile = fullfile(outDir, 'fmac_2p2z_coefficients.h');
fid = fopen(outFile, 'w');
assert(fid >= 0, 'Cannot create coefficient header');
fprintf(fid, '/* Generated by fmac_2p2z_current_loop.m; verify on hardware. */\n');
fprintf(fid, '#define FMAC_2P2Z_P 3\n#define FMAC_2P2Z_Q 2\n#define FMAC_2P2Z_R %d\n', R);
fprintf(fid, '#define FMAC_2P2Z_PRE_SHIFT %d\n#define FMAC_2P2Z_POST_SHIFT %d\n', preShift, postShift);
fprintf(fid, 'static const int16_t fmac_2p2z_coeff_b[3] = {(int16_t)0x%04X, (int16_t)0x%04X, (int16_t)0x%04X};\n', q15_u16(1), q15_u16(2), q15_u16(3));
fprintf(fid, 'static const int16_t fmac_2p2z_coeff_a[2] = {(int16_t)0x%04X, (int16_t)0x%04X};\n', q15_u16(4), q15_u16(5));
fprintf(fid, '/* X2=[B0 B1 B2 A1 A2], X1_BASE=%d, X1_SIZE=%d, Y_BASE=%d, Y_SIZE=%d */\n', mem.X1_BASE, mem.X1_SIZE, mem.Y_BASE, mem.Y_SIZE);
fclose(fid);
save(fullfile(outDir, 'fmac_2p2z_results.mat'), 'cfg', 'adcCountsPerAmp', 'Gplant_s', 'Gplant_norm_s', 'C_s', 'C_z', 'C_fmac_z', 'Loop_z', 'Tclosed', 'B', 'A_fmac', 'q15', 'q15_u16', 'R', 'preShift', 'postShift', 'mem', 'Gm', 'Pm', 'Wcg', 'Wcp');

%% Plots
figure('Name', 'FMAC 2P2Z current loop');
subplot(2,2,1); bode(Gplant_s); grid on; title('Continuous current plant');
subplot(2,2,2); margin(Loop_z); grid on; title('Discrete loop margin');
subplot(2,2,3); step(Tclosed, t); grid on; title('Closed-loop current response');
subplot(2,2,4); pzmap(C_fmac_z); grid on; title('2P2Z controller poles/zeros');
fprintf('Generated: %s\n', outFile);
