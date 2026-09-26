function results = digital_power_current_loop_design_final()
% DIGITAL_POWER_CURRENT_LOOP_DESIGN_FINAL
% STM32G474 + FMAC / 4-switch synchronous bidirectional buck-boost
% 200 kHz PWM / 100 kHz current-loop execution / 10 kHz voltage-loop
% averaged-model design and FMAC export.
%
% IMPORTANT:
% This is a control-design averaged model. It is NOT a switching-level
% verification model. Dead time, reverse recovery, nonlinear Coss,
% propagation delay, PCB parasitics and COMP blanking must be verified
% separately.
%
% Final hardware baseline used here:
%   Vin      = 5...40 V
%   Vout     = 0.5...40 V
%   fsw      = 200 kHz
%   L        = 22 uH
%   L DCR    = 2.5 mOhm
%   Cout     = 630 uF
%   Cout ESR = 25 mOhm (editable; replace with measured effective ESR)
%   Rs       = 7 mOhm
%   INA gain = 20 V/V
%   Isoft    = 10 A
%   HW OCP   = 11 A
%
% Model convention:
%   Each physical direction is represented in a local source->load frame.
%   The plant is always:
%
%       diL/dt = (dA*Vs - dB*Vo - rL*iL)/L
%       dVo/dt = (dB*iL - iLoad)/C
%
%   BUCK:  dA=D, dB=1
%   BOOST: dA=1, dB=1-D
%
% Reverse operation swaps physical ports before entering this local frame.
% Therefore the four modes are all handled by the same equations.
%
% Controller:
%   controlled variable = output/load current, NOT inductor current.
%   normalized error = (Iref - Iout)/Ibase
%   controller output = normalized duty perturbation
%
% Type-II:
%   C(s) = K*(1+s/wz)/(s*(1+s/wp))
%   discretized by Tustin to a 2P2Z form.
%
% FMAC:
%   B = [b0 b1 b2]
%   A = [-a1 -a2]
%   y = 2^R * (B*x_history + A*y_history)
%
% Outputs:
%   current_loop_full_scan_final.csv
%   current_loop_fmac_coeff_final.h
%
% Requires:
%   MATLAB + Control System Toolbox

clc;
fprintf('\n============================================================\n');
fprintf(' STM32G474 / FMAC CURRENT LOOP DESIGN - FINAL\n');
fprintf(' 4-switch bidirectional Buck-Boost / unified averaged model\n');
fprintf('============================================================\n\n');

P = default_params();

%% Hardware checks
fprintf('--- Hardware / scaling checks ---\n');
senseGain = P.Rs * P.INA_gain;
IadcPos = (P.ADC_FS - P.VREF165) / senseGain;
IadcNeg = P.VREF165 / senseGain;

fprintf('Current sense gain          = %.4f V/A\n', senseGain);
fprintf('Current @ +10 A             = %.4f V\n', P.VREF165 + 10*senseGain);
fprintf('Current @ -10 A             = %.4f V\n', P.VREF165 - 10*senseGain);
fprintf('ADC positive current limit  = %.3f A\n', IadcPos);
fprintf('ADC negative current limit  = %.3f A\n', IadcNeg);
fprintf('ADC usable symmetric limit  = +/- %.3f A\n', min(IadcPos,IadcNeg));

if min(IadcPos,IadcNeg) < P.Iocp_hw
    warning('ADC range does not reach the 11 A hardware OCP threshold. Keep COMP OCP independent.');
end

fprintf('fsw                         = %.1f kHz\n', P.fsw/1e3);
fprintf('current-loop rate           = %.1f kHz\n', P.Fcurrent/1e3);
fprintf('voltage-loop rate           = %.1f kHz\n', P.Fvoltage/1e3);
fprintf('current-loop Ts             = %.3f us\n', P.Ts*1e6);
fprintf('L                           = %.3f uH\n', P.L*1e6);
fprintf('L DCR                       = %.3f mOhm\n', P.RL_DCR*1e3);
fprintf('Cout                        = %.1f uF\n', P.C*1e6);
fprintf('Cout ESR                    = %.3f mOhm\n', P.ESR*1e3);
fprintf('Ibase                       = %.2f A\n', P.Ibase);
fprintf('DutyScale                   = %.3f\n\n', P.DutyScale);

%% Operating points
ops = build_operating_points(P);
fprintf('--- Operating-point scan ---\n');
fprintf('Total operating points: %d\n', height(ops));

nomIdx = find(strcmp(ops.Mode,'FWD_BUCK') & ...
              abs(ops.Vs-24)<1e-9 & ...
              abs(ops.Vo-12)<1e-9 & ...
              abs(ops.Io-5)<1e-9,1);
if isempty(nomIdx)
    nomIdx = 1;
end
nom = ops(nomIdx,:);
fprintf('Nominal local-frame point: %s, Vs=%.2f V, Vo=%.2f V, Io=%.2f A, D=%.6f\n\n', ...
    nom.Mode{1},nom.Vs,nom.Vo,nom.Io,nom.D);

%% Linearize every operating point
data = repmat(empty_result(),height(ops),1);
for k = 1:height(ops)
    data(k) = analyze_operating_point(table2struct(ops(k,:)),P);
end

fprintf('--- Plant scan ---\n');
print_scan_summary(data);

%% Controller synthesis
fprintf('\n--- Type-II controller synthesis ---\n');

buckIdx = contains({data.Mode},'_BUCK');
boostIdx = contains({data.Mode},'_BOOST');

designBuck  = design_group_controller(data(buckIdx), P, 'BUCK');
designBoost = design_group_controller(data(boostIdx),P, 'BOOST');

% Diagnostic only: one controller across every mode.
designUniversal = design_group_controller(data,P,'UNIVERSAL');

print_design('BUCK controller',designBuck);
print_design('BOOST controller',designBoost);
print_design('UNIVERSAL diagnostic',designUniversal);

%% Nonlinear averaged verification
fprintf('\n--- Nonlinear averaged-loop verification ---\n');
tests = representative_tests(data,P);

nl = repmat(empty_nonlinear_result(),numel(tests),1);
for k = 1:numel(tests)
    if strcmp(region_from_mode(tests(k).Mode),'BUCK')
        d = designBuck;
    else
        d = designBoost;
    end
    nl(k) = nonlinear_test(tests(k),P,d);
    fprintf('%-12s Vs=%6.1f Vo=%6.1f I=%.1f A : peakErr=%.4f A, finalErr=%.4f A, sat=%d\n', ...
        tests(k).Mode,tests(k).Vs,tests(k).Vo,tests(k).Io, ...
        nl(k).peakError,nl(k).finalError,nl(k).dutySaturated);
end

%% FMAC export
fprintf('\n--- FMAC coefficient generation ---\n');

fmacBuck  = make_fmac(designBuck,P,'BUCK');
fmacBoost = make_fmac(designBoost,P,'BOOST');

print_fmac('BUCK',fmacBuck);
print_fmac('BOOST',fmacBoost);

%% Export
base = fileparts(mfilename('fullpath'));
if isempty(base)
    base = pwd;
end

csvFile = fullfile(base,'current_loop_full_scan_100k.csv');
headerFile = fullfile(base,'current_loop_fmac_coeff_100k.h');

save_scan_csv(data,csvFile);
write_fmac_header(fmacBuck,fmacBoost,designBuck,designBoost,P,headerFile);

fprintf('\n--- Export ---\n');
fprintf('Scan CSV    : %s\n',csvFile);
fprintf('FMAC header : %s\n',headerFile);

%% Results
results = struct();
results.Parameters = P;
results.OperatingPoints = ops;
results.Scan = data;
results.Buck = designBuck;
results.Boost = designBoost;
results.Universal = designUniversal;
results.Nonlinear = nl;
results.FMAC_Buck = fmacBuck;
results.FMAC_Boost = fmacBoost;

fprintf('\nDONE.\n');
fprintf('The generated FMAC coefficients are valid only with the exact\n');
fprintf('normalization/sign convention documented in the generated header.\n');
fprintf('Hardware COMP OCP remains independent of ADC/software control.\n\n');

end


%% ========================================================================
function P = default_params()

% Power stage
P.fsw = 200e3;
P.Fcurrent = 100e3;
P.Fvoltage = 10e3;
P.Ts_pwm = 1/P.fsw;
P.Ts = 1/P.Fcurrent;

P.L = 15e-6;
P.RL_DCR = 6.5e-3;

P.C = 470e-6;
P.ESR = 25e-3;

% Current sense
P.Rs = 8e-3;
P.INA_gain = 20;
P.VREF165 = 1.65;
P.ADC_FS = 3.3;

% Voltage divider retained as a monitoring parameter.
% Replace with the actual final divider if needed.
P.RU = 75e3;
P.RL = 3e3;

% Current limits
P.Isoft = 10;
P.Iocp_hw = 11;
P.I_L_sat = 12;
P.I_L_thermal = 13;

% Final operating range
P.VinMin = 12;
P.VinMax = 70;
P.VoutMin = 1;
P.VoutMax = 70;
% Limit BOOST operation to at most a 2:1 output-to-input voltage ratio.
P.BoostRatioMax = 2;

% PWM usable range
P.Dmin = 0;
P.Dmax = 1;

% Controller design target.
% Keep crossover comfortably below fsw/10 initially.
P.fc_target = 4e3;
P.PM_target = 60;
P.PM_min = 50;
P.GM_min = 6;

% Frequency sweep
P.fmin = 5;
P.fmax = 0.49*P.Fcurrent;
P.Nfreq = 800;

% Digital delay model:
% One PWM period is a conservative first model.
P.Tdelay = 1.0*P.Ts;

% Operating-point scan covers the actual hardware range. Topology, duty,
% voltage-ratio, and current limits below remove unreachable combinations.
P.VinList  = [12 15 20 24 28 32 36 40 48 56 60 64 70];
P.VoutList = [1 2 3.3 5 8 12 16 20 24 28 32 36 40 48 56 60 64 70];
P.IoutList = [0.5 1 2 3 5 7 8 10];

% Load model used for controller synthesis.
% The plant output is IOUT. For a resistive load, IOUT = Vout/R.
P.LoadModels = {'RESISTIVE','CONSTANT_CURRENT','CONSTANT_POWER'};
P.DesignLoadModel = 'RESISTIVE';

% Type-II controller search grid. The same grid is used for BUCK, BOOST,
% and the diagnostic universal controller search.
P.fc_candidates = [0.5 0.75 1 1.5 2 2.5 3 4 5 6 8]*1e3;
P.fz_ratio = [0.05 0.08 0.1 0.15 0.2 0.3 0.5 0.8];
P.fp_ratio = [2 3 4 5 7 10];
P.MaxControllerSearchPoints = 48;

% Numerical linearization
P.dx_rel = 1e-6;
P.dd = 1e-6;

% Nonlinear averaged simulation
P.simT = 0.015;
P.IrefStep = 0.5;
P.dutySlew = Inf;

% Current normalization:
% +10 A -> +1.0
% -10 A -> -1.0
P.Ibase = 10;

% Controller output normalization:
% y=+1 corresponds to +DutyScale duty perturbation.
P.DutyScale = 0.20;

end


%% ========================================================================
function ops = build_operating_points(P)

rows = {};

for dirCell = {'FWD','REV'}
    dirName = dirCell{1};

    for regionCell = {'BUCK','BOOST'}
        region = regionCell{1};
        mode = [dirName '_' region];

        for vs = P.VinList
            for vo = P.VoutList

                % Local source-to-load frame requires Vs > Vo for BUCK
                % and Vo > Vs for BOOST.
                if strcmp(region,'BUCK')
                    if vo >= vs
                        continue;
                    end
                else
                    if vo <= vs
                        continue;
                    end
                    if vo/vs > P.BoostRatioMax
                        continue;
                    end
                end

                for io = P.IoutList

                    if io > P.Isoft
                        continue;
                    end

                    % Find the actual DC-balanced duty including inductor DCR.
                    [D, iLdc, valid] = dc_operating_point(vs,vo,io,region,P);

                    if ~valid
                        continue;
                    end

                    % Reject unrealistic inductor current.
                    if abs(iLdc) > P.I_L_sat
                        continue;
                    end

                    rows(end+1,:) = {mode,vs,vo,io,D,iLdc}; %#ok<AGROW>
                end
            end
        end
    end
end

ops = cell2table(rows, ...
    'VariableNames',{'Mode','Vs','Vo','Io','D','ILdc'});

end


%% ========================================================================
function [D,iLdc,valid] = dc_operating_point(Vs,Vo,Io,region,P)

valid = true;

switch upper(region)

    case 'BUCK'
        % Output current approximately equals inductor current in the
        % local averaged DC model.
        iLdc = Io;

        % D*Vs = Vo + rL*iL
        D = (Vo + P.RL_DCR*iLdc)/Vs;

    case 'BOOST'
        % Vo-side current = (1-D)*iL.
        % Io = (1-D)*iL -> iL = Io/(1-D)
        %
        % Steady state:
        % Vs - (1-D)*Vo - rL*iL = 0
        %
        % Solve algebraically:
        % Vs - x*Vo - rL*Io/x = 0
        % x=(1-D)
        %
        % Vo*x^2 - Vs*x + rL*Io = 0
        a = Vo;
        b = -Vs;
        c = P.RL_DCR*Io;

        disc = b^2 - 4*a*c;

        if disc <= 0
            valid = false;
            D = NaN;
            iLdc = NaN;
            return;
        end

        x1 = (-b + sqrt(disc))/(2*a);
        x2 = (-b - sqrt(disc))/(2*a);

        candidates = [x1 x2];
        candidates = candidates(candidates > 0 & candidates < 1);

        if isempty(candidates)
            valid = false;
            D = NaN;
            iLdc = NaN;
            return;
        end

        % Physical boost solution close to the ideal x=Vs/Vo.
        xideal = Vs/Vo;
        [~,idx] = min(abs(candidates-xideal));
        x = candidates(idx);

        D = 1-x;
        iLdc = Io/x;

    otherwise
        error('Unknown region.');
end

if ~isfinite(D) || ~isfinite(iLdc)
    valid = false;
end

if D <= P.Dmin || D >= P.Dmax
    valid = false;
end

end


%% ========================================================================
function r = empty_result()

r = struct( ...
    'Mode','', ...
    'Vs',NaN, ...
    'Vo',NaN, ...
    'Io',NaN, ...
    'D',NaN, ...
    'ILdc',NaN, ...
    'LoadModel','', ...
    'FeedbackSensor','', ...
    'Gplant',tf(0), ...
    'PlantDCGain',NaN, ...
    'fc_max',NaN, ...
    'PM',NaN, ...
    'GM',NaN, ...
    'Stable',false, ...
    'Rload',NaN, ...
    'VtermDC',NaN, ...
    'Poles',[]);

end


%% ========================================================================
function r = analyze_operating_point(op,P)

r = empty_result();

r.Mode = op.Mode;
r.Vs = op.Vs;
r.Vo = op.Vo;
r.Io = op.Io;
r.D = op.D;
r.ILdc = op.ILdc;
r.LoadModel = P.DesignLoadModel;
r.FeedbackSensor = output_current_sensor(op.Mode);

Rload = op.Vo/max(op.Io,1e-9);
r.Rload = Rload;

% At DC, capacitor current is zero, so ESR drop is zero and the terminal
% voltage equals the specified operating-point voltage.
r.VtermDC = op.Vo;

% State:
%   x1 = inductor current
%   x2 = capacitor voltage
%
% Numerically linearize the averaged nonlinear equations around the TRUE
% DC-balanced operating point.
x0 = [op.ILdc; op.Vo];

A = zeros(2,2);
B = zeros(2,1);

dx = [max(P.dx_rel*max(abs(op.ILdc),1),1e-7); ...
      max(P.dx_rel*max(abs(op.Vo),1),1e-7)];

for j = 1:2
    xp = x0;
    xm = x0;

    xp(j) = xp(j) + dx(j);
    xm(j) = xm(j) - dx(j);

    fp = avg_model(xp,op.D,op,P,P.DesignLoadModel);
    fm = avg_model(xm,op.D,op,P,P.DesignLoadModel);

    A(:,j) = (fp-fm)/(2*dx(j));
end

fp = avg_model(x0,op.D+P.dd,op,P,P.DesignLoadModel);
fm = avg_model(x0,op.D-P.dd,op,P,P.DesignLoadModel);

B = (fp-fm)/(2*P.dd);

% Linearize the externally measured output-port current around the DC point.
% The inductor current is an internal model state, never the feedback signal.
Cmat = zeros(1,2);
for j = 1:2
    xp = x0;
    xm = x0;
    xp(j) = xp(j) + dx(j);
    xm(j) = xm(j) - dx(j);
    [~,yp] = avg_model(xp,op.D,op,P,P.DesignLoadModel);
    [~,ym] = avg_model(xm,op.D,op,P,P.DesignLoadModel);
    Cmat(j) = (yp-ym)/(2*dx(j));
end
[~,yp] = avg_model(x0,op.D+P.dd,op,P,P.DesignLoadModel);
[~,ym] = avg_model(x0,op.D-P.dd,op,P,P.DesignLoadModel);
Dmat = (yp-ym)/(2*P.dd);

sys = ss(A,B,Cmat,Dmat);
r.Gplant = minreal(tf(sys));

r.PlantDCGain = dcgain(r.Gplant);
r.Poles = pole(r.Gplant);

r.Stable = all(isfinite(r.Poles)) && all(real(r.Poles)<0);

% Plant-only -3 dB estimate relative to low-frequency gain.
try
    w = logspace(log10(2*pi*P.fmin), ...
                 log10(2*pi*P.fmax),P.Nfreq).';
    H = squeeze(freqresp(r.Gplant,w));
    mag = abs(H(:));

    ref = mag(1);
    idx = find(mag <= ref/sqrt(2),1,'first');

    if isempty(idx)
        r.fc_max = NaN;
    else
        r.fc_max = w(idx)/(2*pi);
    end
catch
    r.fc_max = NaN;
end

end


%% ========================================================================
function [dA,dB] = duty_factors(D,mode)

region = region_from_mode(mode);

if strcmp(region,'BUCK')
    dA = D;
    dB = 1;
else
    dA = 1;
    dB = 1-D;
end

end


%% ========================================================================
function [dxdt,y] = avg_model(x,D,op,P,loadModel)

iL = x(1);
v = x(2);

D = min(max(D,P.Dmin),P.Dmax);

[dA,dB] = duty_factors(D,op.Mode);

% Inductor equation.
di = (dA*op.Vs - dB*v - P.RL_DCR*iL)/P.L;

% Load model.
%
% For a resistive load, explicitly include capacitor ESR:
%
%   vterm = vC + ESR*iC
%   iC    = iConv - vterm/R
%
% Therefore:
%
%   iC = (iConv - vC/R) / (1 + ESR/R)
%
% This makes Cout ESR part of the small-signal plant rather than a
% parameter that is merely reported.
switch upper(loadModel)

    case 'RESISTIVE'
        R = op.Vo/max(op.Io,1e-9);
        iConv = dB*iL;

        iC = (iConv - v/R) / (1 + P.ESR/R);
        iload = iConv - iC;

    case 'CONSTANT_CURRENT'
        % Constant-current load has no incremental load conductance.
        % ESR changes terminal voltage but not the current drawn by the
        % ideal current sink.
        iload = op.Io;
        iC = dB*iL - iload;

    case 'CONSTANT_POWER'
        % Solve the algebraic ESR relation:
        %
        %   iC = iConv - Pload/(v + ESR*iC)
        %
        % Use one Newton iteration sequence. For small ESR this converges
        % rapidly and keeps the averaged model nonlinear.
        Pload = op.Vo*op.Io;
        iConv = dB*iL;

        iC = iConv - Pload/max(v,1e-6);

        for iter = 1:4
            vterm = v + P.ESR*iC;
            vterm = max(vterm,1e-3);

            F = iC - iConv + Pload/vterm;
            dF = 1 - Pload*P.ESR/(vterm^2);

            if abs(dF) < 1e-9
                break;
            end

            iC = iC - F/dF;
        end

        iload = Pload/max(v + P.ESR*iC,1e-3);

    otherwise
        error('Unknown load model: %s',loadModel);
end

dv = iC/P.C;

dxdt = [di;dv];

% Physical terminal voltage including capacitor ESR.
vterm = v + P.ESR*iC; %#ok<NASGU>

% Output-current shunt is at the port, outside the local output capacitor.
y = iload;

end


%% ========================================================================
function region = region_from_mode(mode)

if contains(mode,'BUCK')
    region = 'BUCK';
elseif contains(mode,'BOOST')
    region = 'BOOST';
else
    error('Unknown mode: %s',mode);
end

end


%% ========================================================================
function sensor = output_current_sensor(mode)

% The current loop always regulates the output current in the active
% power-flow direction. These are the physical ADC channels used by the
% firmware; their polarity is converted to positive local source-to-load
% current before forming the normalized error.
if startsWith(mode,'FWD_')
    sensor = 'FORWARD_IOUT';
elseif startsWith(mode,'REV_')
    sensor = 'REVERSE_IOUT';
else
    error('Unknown direction in mode: %s',mode);
end

end


%% ========================================================================
function design = design_group_controller(data,P,name)

design = struct();
design.Name = name;
design.Valid = false;
design.Controller = tf(0);
design.K = NaN;
design.fc_design = NaN;
design.fz = NaN;
design.fp = NaN;
design.WorstPM = -Inf;
design.WorstGM = -Inf;
design.WorstFc = NaN;
design.Cost = Inf;
design.FC = [];
design.PM = [];
design.GM = [];

[wSweep,plantResp] = group_frequency_data(data,P);

if isempty(data)
    return;
end

% Reduce candidate points during search, but validate against ALL points
% after a candidate has been selected.
searchIdx = unique(round(linspace(1,numel(data), ...
    min(P.MaxControllerSearchPoints,numel(data)))));
searchPlantResp = plantResp(:,searchIdx);

s = tf('s');

% In the local BOOST convention, increasing D reduces the output-side
% conduction interval. Reverse the controller sign so positive current error
% always commands positive output-current correction.
loopSign = controller_sign(name);

for fc = P.fc_candidates

    for rz = P.fz_ratio

        for rp = P.fp_ratio

            wz = 2*pi*fc*rz;
            wp = 2*pi*fc*rp;

            C0 = (1+s/wz)/(s*(1+s/wp));

            % Normalize K at fc using geometric/median plant magnitude.
            plantMagFc = zeros(numel(searchIdx),1);

            for k = 1:numel(searchIdx)
                H = searchPlantResp(:,k);
                plantMagFc(k) = interp_log_mag( ...
                    wSweep,abs(H),2*pi*fc);
            end

            controllerAtFc = ...
                abs((1+1i*2*pi*fc/wz)/ ...
                (1i*2*pi*fc*(1+1i*2*pi*fc/wp)));

            mags = controllerAtFc*plantMagFc;
            mags = mags(isfinite(mags) & mags>0);

            if isempty(mags)
                continue;
            end

            K = 1/median(mags);
            C = loopSign*K*C0;

            controllerResp = squeeze(freqresp(C0,wSweep));
            controllerResp = controllerResp(:);

            loopResp = loopSign*K*controllerResp.*searchPlantResp;

            [FCs,PMs,GMs] = exact_margins_response_matrix( ...
                loopResp,wSweep,P.Tdelay);

            if any(~isfinite(PMs))
                continue;
            end

            worstPM = min(PMs);
            finiteGM = GMs(isfinite(GMs));

            if isempty(finiteGM)
                worstGM = Inf;
            else
                worstGM = min(finiteGM);
            end

            finiteFC = FCs(isfinite(FCs));
            if isempty(finiteFC)
                medFc = Inf;
            else
                medFc = median(finiteFC);
            end

            cost = ...
                abs(worstPM-P.PM_target) + ...
                max(0,P.PM_min-worstPM)*50 + ...
                max(0,P.GM_min-worstGM)*10 + ...
                abs(medFc-P.fc_target)/1000;

            if worstPM >= P.PM_min && ...
               worstGM >= P.GM_min && ...
               cost < design.Cost

                design.Valid = true;
                design.Controller = C;
                design.K = K;
                design.fc_design = fc;
                design.fz = fc*rz;
                design.fp = fc*rp;
                design.Cost = cost;
            end

        end
    end
end

% If no strict candidate exists, select the least-bad candidate.
if ~design.Valid
    [bestController,bestMeta] = best_effort_controller(data,P,name);

    design.Controller = bestController;
    design.K = bestMeta.K;
    design.fc_design = bestMeta.fc_design;
    design.fz = bestMeta.fz;
    design.fp = bestMeta.fp;
end

% IMPORTANT: final metrics are always calculated against ALL operating
% points, not only the reduced search set.
[design.FC,design.PM,design.GM] = group_metrics( ...
    design.Controller,data,P);

design.WorstFc = min(design.FC);

finitePM = design.PM(isfinite(design.PM));
if isempty(finitePM)
    design.WorstPM = -Inf;
else
    design.WorstPM = min(finitePM);
end

finiteGM = design.GM(isfinite(design.GM));
if isempty(finiteGM)
    design.WorstGM = Inf;
else
    design.WorstGM = min(finiteGM);
end

design.Valid = all(isfinite(design.PM)) && ...
    design.WorstPM >= P.PM_min && ...
    design.WorstGM >= P.GM_min;

end


%% ========================================================================
function [Cbest,d] = best_effort_controller(data,P,name)

s = tf('s');

loopSign = controller_sign(name);

bestCost = Inf;
Cbest = tf(0);

d = struct( ...
    'K',NaN, ...
    'fc_design',NaN, ...
    'fz',NaN, ...
    'fp',NaN);

[wSweep,plantResp] = group_frequency_data(data,P);

searchIdx = unique(round(linspace(1,numel(data), ...
    min(P.MaxControllerSearchPoints,numel(data)))));
searchPlantResp = plantResp(:,searchIdx);

for fc = P.fc_candidates

    for rz = P.fz_ratio

        for rp = P.fp_ratio

            wz = 2*pi*fc*rz;
            wp = 2*pi*fc*rp;

            C0 = (1+s/wz)/(s*(1+s/wp));

            plantMagFc = zeros(numel(searchIdx),1);

            for k=1:numel(searchIdx)
                plantMagFc(k) = interp_log_mag( ...
                    wSweep,abs(searchPlantResp(:,k)),2*pi*fc);
            end

            controllerAtFc = abs( ...
                (1+1i*2*pi*fc/wz)/ ...
                (1i*2*pi*fc*(1+1i*2*pi*fc/wp)));

            mags = controllerAtFc*plantMagFc;
            mags = mags(isfinite(mags) & mags>0);

            if isempty(mags)
                continue;
            end

            K = 1/median(mags);
            C = loopSign*K*C0;

            controllerResp = squeeze(freqresp(C0,wSweep));
            controllerResp = controllerResp(:);

            loopResp = loopSign*K*controllerResp.*searchPlantResp;

            [fcg,pm,gm] = exact_margins_response_matrix( ...
                loopResp,wSweep,P.Tdelay);

            finitePM = pm(isfinite(pm));
            finiteGM = gm(isfinite(gm));
            finiteFC = fcg(isfinite(fcg));

            if isempty(finitePM)
                pmWorst = -Inf;
            else
                pmWorst = min(finitePM);
            end

            if isempty(finiteGM)
                gmWorst = Inf;
            else
                gmWorst = min(finiteGM);
            end

            if isempty(finiteFC)
                fcMed = Inf;
            else
                fcMed = median(finiteFC);
            end

            cost = ...
                max(0,P.PM_min-pmWorst)*100 + ...
                max(0,P.GM_min-gmWorst)*20 + ...
                abs(fcMed-P.fc_target)/1000;

            if cost < bestCost
                bestCost = cost;
                Cbest = C;

                d.K = K;
                d.fc_design = fc;
                d.fz = fc*rz;
                d.fp = fc*rp;
            end

        end
    end
end
end


%% ========================================================================
function signValue = controller_sign(name)

if strcmpi(name,'BOOST')
    signValue = -1;
else
    % BUCK and the mixed UNIVERSAL diagnostic use the local positive sign.
    signValue = 1;
end

end


%% ========================================================================
function v = interp_log_mag(w,mag,wq)

mag = max(mag(:),realmin);
w = w(:);

v = 10.^interp1(log10(w),log10(mag), ...
                log10(wq),'linear','extrap');

end


%% ========================================================================
function [fc,pm,gm] = group_metrics(C,data,P)

fc = zeros(numel(data),1);
pm = zeros(numel(data),1);
gm = zeros(numel(data),1);

for k=1:numel(data)
    [fc(k),pm(k),gm(k)] = exact_margins( ...
        C*data(k).Gplant,P.Tdelay);
end

end


%% ========================================================================
function [w,plantResp] = group_frequency_data(data,P)

w = logspace(log10(2*pi*P.fmin), ...
             log10(2*pi*P.fmax),P.Nfreq).';

plantResp = zeros(numel(w),numel(data));

for k=1:numel(data)
    H = squeeze(freqresp(data(k).Gplant,w));
    plantResp(:,k) = H(:);
end

end


%% ========================================================================
function [fc,pm,gm] = exact_margins(L,Tdelay)

w = logspace(log10(2*pi*5), ...
             log10(2*pi*0.49e5),800).';

resp = squeeze(freqresp(L,w));
resp = resp(:);

[fc,pm,gm] = exact_margins_response(resp,w,Tdelay);

end


%% ========================================================================
function [fc,pm,gm] = exact_margins_response_matrix(resp,w,Tdelay)

nPoints = size(resp,2);

fc = zeros(nPoints,1);
pm = zeros(nPoints,1);
gm = zeros(nPoints,1);

for k=1:nPoints
    [fc(k),pm(k),gm(k)] = exact_margins_response( ...
        resp(:,k),w,Tdelay);
end

end


%% ========================================================================
function [fc,pm,gm] = exact_margins_response(resp,w,Tdelay)

resp = resp(:);
w = w(:);

mag = abs(resp);

% Avoid log(0)
mag = max(mag,realmin);

phase = unwrap(angle(resp))*180/pi;

logm = log10(mag);

% Gain crossover: first descending crossing of 0 dB.
idx = find(logm(1:end-1)>=0 & logm(2:end)<0,1,'first');

if isempty(idx)
    % If there is no crossover in the scan range, report NaN rather than
    % pretending the closest point is a valid crossover.
    fc = NaN;
    pm = NaN;
else
    x1 = logm(idx);
    x2 = logm(idx+1);

    lw1 = log10(w(idx));
    lw2 = log10(w(idx+1));

    lw = lw1 + (0-x1)*(lw2-lw1)/(x2-x1);
    wc = 10^lw;

    ph = interp1(log10(w),phase,log10(wc), ...
                 'linear','extrap');

    % Add explicit digital/control delay.
    ph = ph - wc*Tdelay*180/pi;

    while ph > 0
        ph = ph - 360;
    end

    pm = 180 + ph;
    fc = wc/(2*pi);
end

% Gain margin: phase crossing at -180 deg.
phaseDelay = phase - w*Tdelay*180/pi;

% unwrap again after delay
phaseDelay = unwrap(phaseDelay*pi/180)*180/pi;

q = find(phaseDelay(1:end-1)>-180 & ...
         phaseDelay(2:end)<=-180,1,'first');

if isempty(q)
    gm = Inf;
else
    p1 = phaseDelay(q);
    p2 = phaseDelay(q+1);

    lw1 = log10(w(q));
    lw2 = log10(w(q+1));

    lw = lw1 + (-180-p1)*(lw2-lw1)/(p2-p1);
    wp = 10^lw;

    gp = 10.^interp1(log10(w),logm, ...
                     log10(wp),'linear','extrap');

    gm = 20*log10(1/gp);
end

end


%% ========================================================================
function tests = representative_tests(data,P)

% Select physically useful representative points:
% 24V->12V, 24V->36V, 40V->20V, 20V->40V etc. when present.
preferred = [
    24 12 5
    24 36 5
    40 20 5
    20 40 5
    12 5  5
    5  8  5
    40 5  5
];

tests = repmat(data(1),0,1);

for p=1:size(preferred,1)

    vin = preferred(p,1);
    vout = preferred(p,2);
    io = preferred(p,3);

    idx = find(abs([data.Vs]-vin)<1e-9 & ...
               abs([data.Vo]-vout)<1e-9 & ...
               abs([data.Io]-io)<1e-9,1);

    if ~isempty(idx)
        tests(end+1) = data(idx); %#ok<AGROW>
    end
end

% If none of the preferred points exist, choose a small evenly spaced set.
if isempty(tests)
    idx = unique(round(linspace(1,numel(data),min(8,numel(data)))));
    tests = data(idx);
end

end


%% ========================================================================
function out = empty_nonlinear_result()

out = struct( ...
    't',[], ...
    'i',[], ...
    'duty',[], ...
    'peakError',NaN, ...
    'finalError',NaN, ...
    'dutySaturated',false);

end


%% ========================================================================
function out = nonlinear_test(op,P,design)

C = design.Controller;

dt = P.Ts;
N = round(P.simT/dt);

t = (0:N-1)'*dt;

x = [op.ILdc;op.Vo];

D0 = op.D;

Iref = op.Io;
Iref2 = min(op.Io+P.IrefStep,P.Isoft);

% Digital controller coefficients.
[num,den] = tfdata(c2d(C,P.Ts,'tustin'),'v');

num = num/den(1);
den = den/den(1);

if numel(num)<3
    num(end+1:3)=0;
end
if numel(den)<3
    den(end+1:3)=0;
end

b = num(1:3);
a = den(2:3);

e1=0;
e2=0;
y1=0;
y2=0;

ilog = zeros(N,1);
dlog = zeros(N,1);
Dapplied = D0;

for n=1:N

    if t(n)>0.003
        ref = Iref2;
    else
        ref = Iref;
    end

    % Feedback is measured output-port current, not internal inductor current.
    [~,Imeas] = avg_model(x,Dapplied,op,P,P.DesignLoadModel);
    e = (ref-Imeas)/P.Ibase;

    % Conventional 2P2Z difference equation.
    y = b(1)*e + ...
        b(2)*e1 + ...
        b(3)*e2 - ...
        a(1)*y1 - ...
        a(2)*y2;

    D = D0 + P.DutyScale*y;

    if isfinite(P.dutySlew)
        dDmax = P.dutySlew*dt;
        Dprev = D0;
        D = min(max(D,Dprev-dDmax),Dprev+dDmax);
    end

    D = min(max(D,P.Dmin),P.Dmax);

    dx = avg_model(x,D,op,P,P.DesignLoadModel);

    x = x + dt*dx;

    e2=e1;
    e1=e;

    y2=y1;
    y1=y;

    ilog(n)=Imeas;
    dlog(n)=D;
    Dapplied=D;
end

target = Iref2;

err = ilog-target;

out.t=t;
out.i=ilog;
out.duty=dlog;
out.peakError=max(abs(err));
out.finalError=abs(err(end));
out.dutySaturated=any( ...
    dlog<=P.Dmin+1e-8 | ...
    dlog>=P.Dmax-1e-8);

end


%% ========================================================================
function f = make_fmac(design,P,name)

[num,den] = tfdata(c2d(design.Controller,P.Ts,'tustin'),'v');

num = num/den(1);
den = den/den(1);

if numel(num)<3
    num(end+1:3)=0;
end

if numel(den)<3
    den(end+1:3)=0;
end

b = num(1:3);

% MATLAB convention:
%   y = b0*x + b1*x1 + b2*x2
%       - a1*y1 - a2*y2
%
% FMAC convention:
%   y = B*x + A*yhistory
%
% Therefore:
%   A = [-a1 -a2]
a = -den(2:3);

% Common coefficient scaling.
mx = max(abs([b(:);a(:)]));

if mx==0
    mx=1;
end

R = 0;
scale = 1;

while mx/scale > 0.999969 && R<7
    scale = scale*2;
    R = R+1;
end

if mx/scale > 0.999969
    error('%s: controller coefficients cannot fit FMAC Q1.15 with R<=7.',name);
end

bq = round(b/scale*32768);
aq = round(a/scale*32768);

bq = max(min(bq,32767),-32768);
aq = max(min(aq,32767),-32768);

f = struct();

f.Name = name;
f.B = b;
f.A = a;

f.Bq = int16(bq);
f.Aq = int16(aq);

f.R = uint8(R);
f.Scale = scale;

f.Ts = P.Ts;

f.InputMeaning = ...
    'normalized current error = (Iref-Iout)/10A';

f.OutputMeaning = ...
    'normalized duty perturbation before DutyScale';

f.DutyScale = P.DutyScale;

f.Equation = ...
    'FMAC y = 2^R*(B*Xhistory + A*Yhistory)';

end


%% ========================================================================
function print_scan_summary(data)

stable = [data.Stable];

fprintf('Points analyzed                 : %d\n',numel(data));
fprintf('Stable plant state matrices     : %d/%d\n', ...
    sum(stable),numel(data));

g = [data.PlantDCGain];
g = g(isfinite(g));

if ~isempty(g)
    fprintf('Plant DC gain range             : %.6g ... %.6g A/duty\n', ...
        min(g),max(g));
end

d = [data.D];
fprintf('Duty range                      : %.6f ... %.6f\n', ...
    min(d),max(d));

il = [data.ILdc];
fprintf('DC inductor-current range       : %.4f ... %.4f A\n', ...
    min(il),max(il));

end


%% ========================================================================
function print_design(name,d)

fprintf('\n%s\n',name);

fprintf('  valid target design = %d\n',d.Valid);
fprintf('  K                 = %.9g\n',d.K);
fprintf('  fc design         = %.3f kHz\n',d.fc_design/1e3);
fprintf('  fz                = %.3f Hz\n',d.fz);
fprintf('  fp                = %.3f Hz\n',d.fp);
fprintf('  worst PM          = %.3f deg\n',d.WorstPM);
fprintf('  worst GM          = %.3f dB\n',d.WorstGM);
fprintf('  lowest crossover  = %.3f kHz\n',d.WorstFc/1e3);

if ~d.Valid
    fprintf('  WARNING: strict PM/GM requirements were not met.\n');
    fprintf('           Controller is diagnostic/best-effort only.\n');
end

end


%% ========================================================================
function print_fmac(name,f)

fprintf('%s:\n',name);

fprintf('  B = [%d %d %d]\n', ...
    f.Bq(1),f.Bq(2),f.Bq(3));

fprintf('  A = [%d %d]\n', ...
    f.Aq(1),f.Aq(2));

fprintf('  R = %d\n',f.R);
fprintf('  common coefficient scale = %.9g\n',f.Scale);

end


%% ========================================================================
function save_scan_csv(data,file)

T = struct2table(data);

if ismember('Gplant',T.Properties.VariableNames)
    T.Gplant = [];
end

if ismember('Poles',T.Properties.VariableNames)
    T.Poles = [];
end

writetable(T,file);

end


%% ========================================================================
function write_fmac_header(fb,ff,db,df,P,file)

fid = fopen(file,'w');

assert(fid>0,'Cannot open header for writing.');

fprintf(fid,'/* Auto-generated by digital_power_current_loop_design_final.m */\n');
fprintf(fid,'#pragma once\n');
fprintf(fid,'#include <stdint.h>\n\n');

fprintf(fid,'/*\n');
fprintf(fid,' * STM32G4 FMAC IIR Direct Form 1 / 2P2Z\n');
fprintf(fid,' *\n');
fprintf(fid,' * Standard controller equation:\n');
fprintf(fid,' *   y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2\n');
fprintf(fid,' *\n');
fprintf(fid,' * FMAC coefficients:\n');
fprintf(fid,' *   B = [b0 b1 b2]\n');
fprintf(fid,' *   A = [-a1 -a2]\n');
fprintf(fid,' *\n');
fprintf(fid,' * Input normalization:\n');
fprintf(fid,' *   x = (Iref - Iout) / %.9g A\n',P.Ibase);
fprintf(fid,' *\n');
fprintf(fid,' * Output normalization:\n');
fprintf(fid,' *   physical deltaD = y * %.9g\n',P.DutyScale);
fprintf(fid,' *\n');
fprintf(fid,' * FMAC post shift R is stored separately for each controller.\n');
fprintf(fid,' * Hardware COMP OCP at %.3f A remains independent.\n');
fprintf(fid,' */\n\n',P.Iocp_hw);

write_one(fb,'FWD_BUCK');
write_one(fb,'REV_BUCK');

write_one(ff,'FWD_BOOST');
write_one(ff,'REV_BOOST');

fprintf(fid,'/* Controller design information */\n');
fprintf(fid,'/* BUCK : K=%.9g, fc=%.3f kHz, PM=%.3f deg, GM=%.3f dB */\n', ...
    db.K,db.fc_design/1e3,db.WorstPM,db.WorstGM);

fprintf(fid,'/* BOOST: K=%.9g, fc=%.3f kHz, PM=%.3f deg, GM=%.3f dB */\n', ...
    df.K,df.fc_design/1e3,df.WorstPM,df.WorstGM);

fprintf(fid,'/* Vin range  = %.3f ... %.3f V */\n',P.VinMin,P.VinMax);
fprintf(fid,'/* Vout range = %.3f ... %.3f V */\n',P.VoutMin,P.VoutMax);
fprintf(fid,'/* PWM        = %.3f kHz */\n',P.fsw/1e3);
fprintf(fid,'/* Current loop execution = %.3f kHz */\n',P.Fcurrent/1e3);
fprintf(fid,'/* Voltage loop execution = %.3f kHz */\n',P.Fvoltage/1e3);
fprintf(fid,'/* L          = %.9g H */\n',P.L);
fprintf(fid,'/* Cout       = %.9g F */\n',P.C);
fprintf(fid,'/* Rs         = %.9g Ohm */\n',P.Rs);
fprintf(fid,'/* Isoft      = %.3f A */\n',P.Isoft);
fprintf(fid,'/* HW OCP     = %.3f A */\n',P.Iocp_hw);

fclose(fid);


    function write_one(f,n)

        fprintf(fid,'static const int16_t %s_FMAC_B[3] = {%d,%d,%d};\n', ...
            n,f.Bq(1),f.Bq(2),f.Bq(3));

        fprintf(fid,'static const int16_t %s_FMAC_A[2] = {%d,%d};\n', ...
            n,f.Aq(1),f.Aq(2));

        fprintf(fid,'static const uint8_t %s_FMAC_R = %d;\n\n', ...
            n,f.R);

    end

end
