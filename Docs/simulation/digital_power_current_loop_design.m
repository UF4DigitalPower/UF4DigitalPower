function results = digital_power_current_loop_design()
% DIGITAL_POWER_CURRENT_LOOP_DESIGN
% Final-stage design/verification tool for:
%   STM32G474 + FMAC
%   4-switch synchronous bidirectional buck-boost
%   200 kHz current loop
%
% The model uses one unified averaged power-stage model:
%
%   diL/dt = (dA*Vs - dB*Vo - rL*iL)/L
%   C*dVo/dt = dB*iL - iLoad
%
% with:
%   BUCK  : dA = D, dB = 1
%   BOOST : dA = 1, dB = 1-D
%
% Reverse operation is represented by swapping source/load ports and
% normalizing current positive from source to load. Therefore the same
% plant equations cover all four physical directions:
%   FWD_BUCK, FWD_BOOST, REV_BUCK, REV_BOOST.
%
% The control plant is IOUT / duty perturbation, not inductor current.
% This is important because the actual hardware does NOT sample inductor
% current.
%
% The script:
%   1) checks current-sense / ADC scaling
%   2) builds a unified averaged state-space model
%   3) linearizes IOUT/duty at every operating point numerically
%   4) scans Buck/Boost, forward/reverse, Vin/Vout/current/load
%   5) includes DCR, capacitor ESR, load model and digital delay
%   6) searches Type-II controllers
%   7) evaluates phase/gain margin and crossover
%   8) runs nonlinear averaged current-loop transients
%   9) quantizes a controller for STM32 FMAC Q1.15 + R shift
%  10) exports CSV and a C header
%
% Requires MATLAB + Control System Toolbox.
%
% NOTE:
% This is an averaged control-design model. It is NOT a replacement for
% final switching-waveform verification with parasitics, dead-time,
% reverse recovery, layout inductance, MOSFET nonlinear capacitances,
% driver propagation delays and COMP blanking/filter behavior.

clc;
fprintf('\n============================================================\n');
fprintf(' STM32G474 / FMAC CURRENT LOOP DESIGN - FINAL\n');
fprintf(' Four-switch bidirectional Buck-Boost / unified averaged model\n');
fprintf('============================================================\n\n');

P = default_params();

fprintf('--- Hardware / scaling checks ---\n');
senseGain = P.Rs * P.INA_gain;
IadcFS = (P.ADC_FS - P.VREF165) / senseGain;
fprintf('Current sense gain = %.4f V/A\n', senseGain);
fprintf('10 A sense voltage  = %.4f V above/below VREF\n', 10*senseGain);
fprintf('11 A sense voltage  = %.4f V above/below VREF\n', 11*senseGain);
fprintf('ADC measurable current (3.2806V max) ~= %.3f A\n', IadcFS);
if IadcFS < P.Iocp_hw
    warning('ADC full-scale current is below hardware OCP current. Hardware COMP protection must be independent of ADC.');
end
fprintf('Voltage divider = %.1f:1, 72 V -> %.3f V ADC\n', ...
    P.RU/P.RL, 72/(P.RU/P.RL));
fprintf('Switching frequency = %.1f kHz, Ts = %.3f us\n', ...
    P.fsw/1e3, P.Ts*1e6);
fprintf('L = %.3f uH, DCR = %.3f mOhm, C = %.1f uF, ESR = %.3f mOhm\n\n', ...
    P.L*1e6, P.RL_DCR*1e3, P.C*1e6, P.ESR*1e3);

ops = build_operating_points(P);
nOp = height(ops);

fprintf('--- Operating-point scan ---\n');
fprintf('Total operating points: %d\n', nOp);

% Nominal point
nomIdx = find(strcmp(ops.Mode,'FWD_BUCK') & abs(ops.Vs-24)<1e-9 & ...
    abs(ops.Vo-12)<1e-9 & abs(ops.Io-5)<1e-9,1);
if isempty(nomIdx), nomIdx = 1; end

nom = ops(nomIdx,:);
fprintf('Nominal: %s, Vs=%.2f V, Vo=%.2f V, Io=%.2f A, D=%.5f\n\n', ...
    nom.Mode{1},nom.Vs,nom.Vo,nom.Io,nom.D);

% Linearize every point
data = repmat(empty_result(),nOp,1);
for k = 1:nOp
    op = table2struct(ops(k,:));
    data(k) = analyze_operating_point(op,P);
end

fprintf('--- Plant / stability scan ---\n');
print_scan_summary(data);

% Design controllers separately for Buck and Boost. A single controller
% across both regions is attempted only as a diagnostic.
fprintf('\n--- Type-II controller synthesis ---\n');
buckIdx = strcmp({data.Mode},'FWD_BUCK') | strcmp({data.Mode},'REV_BUCK');
boostIdx = strcmp({data.Mode},'FWD_BOOST') | strcmp({data.Mode},'REV_BOOST');

designBuck = design_group_controller(data(buckIdx),P,'BUCK');
designBoost = design_group_controller(data(boostIdx),P,'BOOST');

% Universal diagnostic
designUniversal = design_group_controller(data,P,'UNIVERSAL');

print_design('BUCK controller',designBuck);
print_design('BOOST controller',designBoost);
print_design('UNIVERSAL diagnostic',designUniversal);

% Nominal nonlinear tests
fprintf('\n--- Nonlinear averaged-loop verification ---\n');
tests = representative_tests(data,P);
nl = repmat(struct('t',[],'i',[],'duty',[],'peakError',[], ...
    'finalError',[],'dutySaturated',[]),numel(tests),1);
for k = 1:numel(tests)
    nl(k) = nonlinear_test(tests(k),P,select_controller(tests(k),designBuck,designBoost));
    fprintf('%-12s Vs=%6.1f Vo=%6.1f I=%.1fA : peak=%.3fA, final=%.3fA, sat=%d\n', ...
        tests(k).Mode,tests(k).Vs,tests(k).Vo,tests(k).Io, ...
        nl(k).peakError,nl(k).finalError,nl(k).dutySaturated);
end

% FMAC export for both controllers
fprintf('\n--- FMAC coefficient generation ---\n');
fmacBuck = make_fmac(designBuck,P,'BUCK');
fmacBoost = make_fmac(designBoost,P,'BOOST');

fprintf('BUCK : Q15 B = [%d %d %d], A = [%d %d], R = %d\n', ...
    fmacBuck.Bq(1),fmacBuck.Bq(2),fmacBuck.Bq(3), ...
    fmacBuck.Aq(1),fmacBuck.Aq(2),fmacBuck.R);
fprintf('BOOST: Q15 B = [%d %d %d], A = [%d %d], R = %d\n', ...
    fmacBoost.Bq(1),fmacBoost.Bq(2),fmacBoost.Bq(3), ...
    fmacBoost.Aq(1),fmacBoost.Aq(2),fmacBoost.R);

% Export
base = fileparts(mfilename('fullpath'));
if isempty(base), base = pwd; end
csvFile = fullfile(base,'current_loop_full_scan.csv');
headerFile = fullfile(base,'current_loop_fmac_coeff_final.h');
save_scan_csv(data,csvFile);
write_fmac_header(fmacBuck,fmacBoost,designBuck,designBoost,headerFile);

fprintf('\n--- Export ---\n');
fprintf('Scan CSV   : %s\n',csvFile);
fprintf('FMAC header: %s\n',headerFile);

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
fprintf('Important: FMAC coefficients are valid only with the exact input/output\n');
fprintf('normalization and sign convention documented in the generated header.\n');
fprintf('The hardware 11 A COMP protection remains independent of ADC/software.\n\n');

end

function P = default_params()
P.fsw = 200e3;
P.Ts = 1/P.fsw;
P.Tdelay = 1.0*P.Ts;

P.L = 15e-6;
P.RL_DCR = 6.5e-3;
P.C = 470e-6;
P.ESR = 25e-3;

P.Rs = 8e-3;
P.INA_gain = 20;
P.VREF165 = 1.65;
P.ADC_FS = 3.2806;
P.RU = 75e3;
P.RL = 3e3;

P.Isoft = 10;
P.Iocp_hw = 11;
P.I_L_sat = 13;
P.I_L_thermal = 16;

P.Vmin = 7;
P.Vmax = 72;
P.Dmin = 0.02;
P.Dmax = 0.98;

% Type-II starting targets
P.fc_target = 12e3;
P.PM_target = 60;
P.PM_min = 50;
P.GM_min = 6;

% Sweep
P.VsList = [7 12 24 48 72];
P.VoList = [5 8 12 24 36 48 60 72];
P.IoList = [1 3 5 8 10];

% load model:
% RESISTIVE uses R=Vo/Io.
% CONSTANT_CURRENT uses incremental conductance 0.
% CONSTANT_POWER uses incremental conductance -Io/Vo.
P.LoadModels = {'RESISTIVE','CONSTANT_CURRENT','CONSTANT_POWER'};
P.DesignLoadModel = 'RESISTIVE';

P.fc_candidates = [5 7.5 10 12 15 18 20 25]*1e3;
P.fz_ratio = [0.05 0.08 0.1 0.15 0.2 0.3 0.5 0.8];
P.fp_ratio = [2 3 4 5 7 10];

% State-space linearization perturbations
P.dx_rel = 1e-6;
P.dd = 1e-6;

% nonlinear simulation
P.simFs = P.fsw;
P.simT = 0.015;
P.IrefStep = 0.5;
P.dutySlew = Inf;
P.dutyNoise = 0;

% FMAC normalization:
% controller is designed for normalized error e_norm and produces normalized
% duty perturbation du_norm. Physical duty perturbation is:
%   deltaD = du_norm * P.DutyScale
P.Ibase = 10;
P.DutyScale = 0.20;  % +/-0.20 duty command range represented by normalized y=1
P.FMAC_inputScale = 1.0;
P.FMAC_outputScale = 1.0;
end

function ops = build_operating_points(P)
rows = {};
for dir = {'FWD','REV'}
    for region = {'BUCK','BOOST'}
        mode = [dir{1} '_' region{1}];
        for vs = P.VsList
            for vo = P.VoList
                if vo < P.Vmin || vo > P.Vmax || vs < P.Vmin || vs > P.Vmax
                    continue;
                end
                D = vo/vs;
                if strcmp(region,'BOOST')
                    D = 1-vs/vo;
                end
                if D <= P.Dmin || D >= P.Dmax
                    continue;
                end
                for io = P.IoList
                    % Reject points that cannot be represented by the
                    % synchronous four-switch operating region.
                    if io > P.Isoft
                        continue;
                    end
                    rows(end+1,:) = {mode,vs,vo,io,D}; %#ok<AGROW>
                end
            end
        end
    end
end
ops = cell2table(rows,'VariableNames',{'Mode','Vs','Vo','Io','D'});
end

function r = empty_result()
r = struct('Mode','','Vs',NaN,'Vo',NaN,'Io',NaN,'D',NaN, ...
    'LoadModel','','Gplant',tf(0),'PlantDCGain',NaN,'fc_max',NaN, ...
    'PM',NaN,'GM',NaN,'Stable',false,'Rload',NaN);
end

function r = analyze_operating_point(op,P)
r = empty_result();
r.Mode = op.Mode;
r.Vs = op.Vs; r.Vo = op.Vo; r.Io = op.Io; r.D = op.D;
r.LoadModel = P.DesignLoadModel;

Rload = op.Vo/op.Io;
r.Rload = Rload;

% Linearize the unified averaged model numerically.
% State x=[iL; vC], input u=D.
[f0,y0] = avg_model([op.Io;op.Vo],op.D,op,P,P.DesignLoadModel);
A = zeros(2); B = zeros(2,1);
dx = [max(P.dx_rel*max(abs(op.Io),1),1e-7); ...
      max(P.dx_rel*max(abs(op.Vo),1),1e-7)];

for j=1:2
    xp=[op.Io;op.Vo]; xm=xp;
    xp(j)=xp(j)+dx(j); xm(j)=xm(j)-dx(j);
    fp = avg_model(xp,op.D,op,P,P.DesignLoadModel);
    fm = avg_model(xm,op.D,op,P,P.DesignLoadModel);
    A(:,j)=(fp-fm)/(2*dx(j));
end
fp = avg_model([op.Io;op.Vo],op.D+P.dd,op,P,P.DesignLoadModel);
fm = avg_model([op.Io;op.Vo],op.D-P.dd,op,P,P.DesignLoadModel);
B=(fp-fm)/(2*P.dd);

% Output is physical load/output current. For resistive load:
% y = v/R. For constant-current load, small-signal output current is
% governed by capacitor/load interaction; use terminal current from
% converter branch dB*iL as the controlled quantity.
if strcmp(P.DesignLoadModel,'RESISTIVE')
    Cmat = [0 1/Rload];
else
    Cmat = [0 0];
end
Dmat = 0;

if strcmp(region_from_mode(op.Mode),'BOOST')
    % Converter output current is dB*iL = (1-D)*iL.
    % Small-signal output-current direct feedthrough wrt duty = -iL.
    Cmat = [1-op.D, 0];
    Dmat = -op.Io;
else
    % Buck output current approximately equals iL at averaged level.
    Cmat = [1,0];
    Dmat = 0;
end

sys = ss(A,B,Cmat,Dmat);
r.Gplant = tf(sys);
r.PlantDCGain = dcgain(r.Gplant);
statePoles = eig(A);
r.Stable = all(isfinite(statePoles)) && all(real(statePoles) < 0);

% Crude plant-only usable bandwidth indicator.
try
    [mag,ph,w] = bode(r.Gplant,{2*pi*10,2*pi*P.fsw/4});
    mag=squeeze(mag); ph=squeeze(ph); w=squeeze(w);
    r.fc_max = max(w(mag > mag(1)/sqrt(2)))/(2*pi);
catch
    r.fc_max = NaN;
end
end

function [dxdt,y] = avg_model(x,D,op,P,loadModel)
iL=x(1); v=x(2);
D=min(max(D,P.Dmin),P.Dmax);

region = region_from_mode(op.Mode);
if strcmp(region,'BUCK')
    dA=D;
    dB=1;
else
    dA=1;
    dB=1-D;
end

di = (dA*op.Vs - dB*v - P.RL_DCR*iL)/P.L;

switch upper(loadModel)
    case 'RESISTIVE'
        R=op.Vo/max(op.Io,1e-6);
        iload=v/R;
        Gload=1/R;
    case 'CONSTANT_CURRENT'
        iload=op.Io;
        Gload=0;
    case 'CONSTANT_POWER'
        Pload=op.Vo*op.Io;
        iload=Pload/max(v,1e-6);
        Gload=-Pload/max(v,1e-6)^2;
    otherwise
        error('Unknown load model.');
end

dv = (dB*iL-iload)/P.C;

dxdt=[di;dv];

% Output current of converter into output capacitor/load.
y=dB*iL;
end

function region = region_from_mode(mode)
if contains(mode,'BUCK')
    region='BUCK';
else
    region='BOOST';
end
end

function design = design_group_controller(data,P,name)
design = struct();
design.Name=name;
design.Valid=false;
design.Controller=tf(0);
design.Candidates=[];
design.WorstPM=-Inf;
design.WorstGM=-Inf;
design.WorstFc=NaN;
design.Cost=Inf;
[wSweep,plantResp] = group_frequency_data(data);
searchIdx=unique(round(linspace(1,numel(data),min(48,numel(data)))));
searchPlantResp=plantResp(:,searchIdx);

% Search one controller for the group. For UNIVERSAL, this is intentionally
% strict; failure means gain scheduling is recommended.
for fc = P.fc_candidates
    for rz = P.fz_ratio
        for rp = P.fp_ratio
            wz=2*pi*fc*rz;
            wp=2*pi*fc*rp;
            s=tf('s');
            C0=(1+s/wz)/(s*(1+s/wp));

            % Gain normalized at the design frequency using the cached plant
            % response instead of repeatedly calling freqresp for each point.
            plantMagFc=10.^interp1(log10(wSweep),log10(abs(searchPlantResp)), ...
                log10(2*pi*fc),'linear','extrap');
            controllerAtFc=(1+1i*2*pi*fc/wz)/ ...
                (1i*2*pi*fc*(1+1i*2*pi*fc/wp));
            mags=abs(controllerAtFc)*plantMagFc(:);
            mags=mags(isfinite(mags) & mags>0);
            if isempty(mags), continue; end
            K=1/median(mags);
            C=K*C0;
            controllerResp=squeeze(freqresp(C0,wSweep));
            controllerResp=controllerResp(:);

            loopResp=K*controllerResp.*searchPlantResp;
            [FC,PM,GM] = exact_margins_response_matrix( ...
                loopResp,wSweep,P.Tdelay);
            ok=all(isfinite(PM));
            if ~ok, continue; end

            worstPM=min(PM);
            worstGM=min(GM);
            medFc=median(FC(isfinite(FC)));

            cost=abs(worstPM-P.PM_target) ...
                + max(0,P.PM_min-worstPM)*50 ...
                + max(0,P.GM_min-worstGM)*10 ...
                + abs(medFc-P.fc_target)/1000;

            if worstPM>=P.PM_min && worstGM>=P.GM_min && cost<design.Cost
                design.Valid=true;
                design.Controller=C;
                design.K=K;
                design.fc_design=fc;
                design.fz=fc*rz;
                design.fp=fc*rp;
                design.WorstPM=worstPM;
                design.WorstGM=worstGM;
                design.WorstFc=min(FC);
                design.Cost=cost;
                design.PM=PM;
                design.GM=GM;
                design.FC=FC;
            end
        end
    end
end

% If strict search fails, keep the least-bad controller so the output is
% still useful for diagnosis rather than silently returning nothing.
if ~design.Valid
    [bestController,bestMeta] = best_effort_controller(data,P);
    design.Controller=bestController;
    design.K=bestMeta.K;
    design.fc_design=bestMeta.fc_design;
    design.fz=bestMeta.fz;
    design.fp=bestMeta.fp;
end
[design.FC,design.PM,design.GM]=group_metrics( ...
    design.Controller,data,P);
design.WorstFc=min(design.FC);
design.WorstPM=min(design.PM);
design.WorstGM=min(design.GM);
design.Valid=all(isfinite(design.PM)) && ...
    all(design.PM>=P.PM_min) && all(design.GM>=P.GM_min);
end

function [Cbest,d] = best_effort_controller(data,P)
s=tf('s');
bestCost=Inf; Cbest=tf(0);
d=struct('K',NaN,'fc_design',NaN,'fz',NaN,'fp',NaN);
[wSweep,plantResp] = group_frequency_data(data);
searchIdx=unique(round(linspace(1,numel(data),min(48,numel(data)))));
searchPlantResp=plantResp(:,searchIdx);
for fc=P.fc_candidates
    for rz=P.fz_ratio
        for rp=P.fp_ratio
            wz=2*pi*fc*rz; wp=2*pi*fc*rp;
            C0=(1+s/wz)/(s*(1+s/wp));
            plantMagFc=10.^interp1(log10(wSweep),log10(abs(searchPlantResp)), ...
                log10(2*pi*fc),'linear','extrap');
            controllerAtFc=(1+1i*2*pi*fc/wz)/ ...
                (1i*2*pi*fc*(1+1i*2*pi*fc/wp));
            mags=abs(controllerAtFc)*plantMagFc(:);
            mags=mags(isfinite(mags) & mags>0);
            if isempty(mags),continue;end
            K=1/median(mags); C=K*C0;
            controllerResp=squeeze(freqresp(C0,wSweep));
            controllerResp=controllerResp(:);
            loopResp=K*controllerResp.*searchPlantResp;
            [fcg,pm,gm] = exact_margins_response_matrix( ...
                loopResp,wSweep,P.Tdelay);
            c=max(0,P.PM_min-min(pm))*100+max(0,P.GM_min-min(gm))*20 ...
                +abs(median(fcg)-P.fc_target)/1000;
            if c<bestCost
                bestCost=c; Cbest=C;
                d.K=K;d.fc_design=fc;d.fz=fc*rz;d.fp=fc*rp;
            end
        end
    end
end
end

function [fc,pm,gm]=group_metrics(C,data,P)
fc=zeros(numel(data),1);pm=fc;gm=fc;
for k=1:numel(data)
    [fc(k),pm(k),gm(k)]=exact_margins(C*data(k).Gplant,P.Tdelay);
end
end

function [w,plantResp]=group_frequency_data(data)
w=logspace(log10(2*pi*5),log10(2*pi*0.49e5),600).';
plantResp=zeros(numel(w),numel(data));
for k=1:numel(data)
    response=squeeze(freqresp(data(k).Gplant,w));
    plantResp(:,k)=response(:);
end
end

function [fc,pm,gm]=exact_margins(L,Tdelay)
% Find gain crossover and phase margin using a frequency sweep.
w=logspace(log10(2*pi*5),log10(2*pi*0.49e5),600).';
resp=squeeze(freqresp(L,w));
resp=resp(:);
[fc,pm,gm]=exact_margins_response(resp,w,Tdelay);
end

function [fc,pm,gm]=exact_margins_response_matrix(resp,w,Tdelay)
% Evaluate a set of SISO loop responses sharing the same frequency grid.
nPoints=size(resp,2);
fc=zeros(nPoints,1);
pm=zeros(nPoints,1);
gm=zeros(nPoints,1);
for k=1:nPoints
    [fc(k),pm(k),gm(k)]=exact_margins_response(resp(:,k),w,Tdelay);
end
end

function [fc,pm,gm]=exact_margins_response(resp,w,Tdelay)
resp=resp(:);
w=w(:);
mag=abs(resp); ph=unwrap(angle(resp))*180/pi;
ph=ph(:);
logm=log10(mag);
logm=logm(:);
idx=find(logm(1:end-1)>=0 & logm(2:end)<0,1,'first');
if isempty(idx)
    [~,ii]=min(abs(logm));
    wc=w(ii);
else
    x1=logm(idx);x2=logm(idx+1);
    lw1=log10(w(idx));lw2=log10(w(idx+1));
    lw=lw1+(0-x1)*(lw2-lw1)/(x2-x1);
    wc=10^lw;
end
phase=interp1(log10(w),ph,log10(wc),'linear','extrap')-wc*Tdelay*180/pi;
while phase>0,phase=phase-360;end
pm=180+phase;
fc=wc/(2*pi);

% Gain margin: locate phase = -180 after adding delay.
phd=ph-w*Tdelay*180/pi;
phd=unwrap(phd*pi/180)*180/pi;
phd=phd(:);
q=find(phd(1:end-1)>-180 & phd(2:end)<=-180,1,'first');
if isempty(q) || q >= numel(w)
    gm=Inf;
else
    % log-frequency interpolation
    p1=phd(q);p2=phd(q+1);
    lw1=log10(w(q));lw2=log10(w(q+1));
    lw=lw1+(-180-p1)*(lw2-lw1)/(p2-p1);
    wp=10^lw;
    gp=10^interp1(log10(w),logm,log10(wp),'linear','extrap');
    gm=20*log10(1/gp);
end
end

function tests=representative_tests(data,P)
keys=[1,3,5,8,10];
tests=data([]);
n=0;
for k=1:numel(data)
    if any(abs(data(k).Io-keys)<1e-9) && ...
       (strcmp(data(k).Mode,'FWD_BUCK') || strcmp(data(k).Mode,'FWD_BOOST') || ...
        strcmp(data(k).Mode,'REV_BUCK') || strcmp(data(k).Mode,'REV_BOOST'))
        % Pick only a manageable representative set.
        if data(k).Vs==24 || data(k).Vs==48 || data(k).Vs==72
            n=n+1; tests(n)=data(k); %#ok<AGROW>
        end
    end
end
if isempty(tests)
    tests=data(1);
end
end

function C=select_controller(op,buck,boost)
if strcmp(region_from_mode(op.Mode),'BUCK')
    C=buck;
else
    C=boost;
end
end

function out=nonlinear_test(op,P,design)
C=design.Controller;
dt=P.Ts;
N=round(P.simT/dt);
t=(0:N-1)'*dt;

x=[op.Io;op.Vo];
D0=op.D;
Iref=op.Io;
Iref2=min(op.Io+P.IrefStep,P.Isoft);

% Extract controller recurrence coefficients in z^-1.
[num,den]=tfdata(c2d(C,P.Ts,'tustin'),'v');
num=num/den(1);den=den/den(1);
b=num(1:3);
a=den(2:3);

e1=0;e2=0;y1=0;y2=0;
ylog=zeros(N,1);dlog=zeros(N,1);

for n=1:N
    if t(n)>0.003
        ref=Iref2;
    else
        ref=Iref;
    end

    e=(ref-x(1))/P.Ibase;

    y=b(1)*e+b(2)*e1+b(3)*e2-a(1)*y1-a(2)*y2;

    % Controller output is normalized duty perturbation.
    D=D0 + P.DutyScale*y;
    D=min(max(D,P.Dmin),P.Dmax);

    dx=avg_model(x,D,op,P,'RESISTIVE');
    x=x+dt*dx;

    e2=e1;e1=e;
    y2=y1;y1=y;
    ylog(n)=x(1);
    dlog(n)=D;
end

target=Iref2;
err=ylog-target;
out.t=t;
out.i=ylog;
out.duty=dlog;
out.peakError=max(abs(err));
out.finalError=abs(err(end));
out.dutySaturated=any(dlog<=P.Dmin+1e-8 | dlog>=P.Dmax-1e-8);
end

function f=make_fmac(design,P,name)
[num,den]=tfdata(c2d(design.Controller,P.Ts,'tustin'),'v');
num=num/den(1); den=den/den(1);

% Ensure exactly 2P2Z.
if numel(num)<3, num(end+1:3)=0; end
if numel(den)<3, den(end+1:3)=0; end

b=num(1:3);
% FMAC equation is y = B*x + A*y'. To implement the standard
% y = B*x - a1*y[n-1] - a2*y[n-2], FMAC A coefficients are -a1,-a2.
a=-den(2:3);

% Find common coefficient scale so all coefficients fit Q1.15.
mx=max(abs([b(:);a(:)]));
if mx==0,mx=1;end
R=0;
scale=1;
while mx/scale > 0.999969 && R<7
    scale=scale*2;
    R=R+1;
end
bq=round(b/scale*32768);
aq=round(a/scale*32768);
bq=max(min(bq,32767),-32768);
aq=max(min(aq,32767),-32768);

f=struct();
f.Name=name;
f.B=b;
f.A=a;
f.Bq=int16(bq);
f.Aq=int16(aq);
f.R=uint8(R);
f.Scale=scale;
f.Ts=P.Ts;
f.InputMeaning='normalized current error (Iref-Iout)/10A';
f.OutputMeaning='normalized duty perturbation before DutyScale';
f.DutyScale=P.DutyScale;
f.Equation='FMAC: y=2^R*(B*X+A*Yprevious), with A=-a_den';
end

function print_scan_summary(data)
stable=[data.Stable];
fprintf('Points analyzed: %d\n',numel(data));
fprintf('Stable plant-only state matrices: %d/%d\n',sum(stable),numel(data));
fprintf('Plant DC gain range: %.4g ... %.4g A/duty\n', ...
    min([data.PlantDCGain]),max([data.PlantDCGain]));
end

function print_design(name,d)
fprintf('\n%s\n',name);
fprintf('  valid target design = %d\n',d.Valid);
fprintf('  K = %.6g, fc target = %.2f kHz\n',d.K,d.fc_design/1e3);
fprintf('  fz = %.2f Hz, fp = %.2f Hz\n',d.fz,d.fp);
fprintf('  worst PM = %.2f deg\n',d.WorstPM);
fprintf('  worst GM = %.2f dB\n',d.WorstGM);
fprintf('  worst/lowest fc = %.2f kHz\n',d.WorstFc/1e3);
if ~d.Valid
    fprintf('  WARNING: no controller met all requested margins. Using best-effort diagnostic controller.\n');
end
end

function save_scan_csv(data,file)
T=struct2table(data);
T.Gplant=[];
writetable(T,file);
end

function write_fmac_header(fb,ff,db,df,file)
fid=fopen(file,'w');
assert(fid>0,'Cannot open header for writing.');

fprintf(fid,'/* Auto-generated by digital_power_current_loop_design.m */\n');
fprintf(fid,'#pragma once\n#include <stdint.h>\n\n');
fprintf(fid,'/*\n');
fprintf(fid,' * FMAC IIR direct-form-1, Q1.15 coefficients.\n');
fprintf(fid,' * STM32G4 FMAC computes Y = 2^R * (B*X + A*Y'') .\n');
fprintf(fid,' * The generated A coefficients are -a1,-a2 so that the\n');
fprintf(fid,' * conventional denominator 1+a1*z^-1+a2*z^-2 is implemented.\n');
fprintf(fid,' * Input = (Iref-Iout)/10A.\n');
fprintf(fid,' * Output = normalized duty perturbation; physical deltaD = output * %.9g.\n',fb.DutyScale);
fprintf(fid,' * R is the FMAC post-shift parameter.\n');
fprintf(fid,' */\n\n');

write_one(fb,'BUCK');
write_one(ff,'BOOST');

fprintf(fid,'/* Controller design information */\n');
fprintf(fid,'/* BUCK: K=%.9g, fc=%.3f kHz, worst PM=%.3f deg, worst GM=%.3f dB */\n', ...
    db.K,db.fc_design/1e3,db.WorstPM,db.WorstGM);
fprintf(fid,'/* BOOST: K=%.9g, fc=%.3f kHz, worst PM=%.3f deg, worst GM=%.3f dB */\n', ...
    df.K,df.fc_design/1e3,df.WorstPM,df.WorstGM);
fprintf(fid,'/* Hardware OCP: %.3f A, independent COMP required. */\n',11);
fclose(fid);

    function write_one(f,n)
        fprintf(fid,'static const int16_t %s_FMAC_B[3] = {%d,%d,%d};\n', ...
            n,f.Bq(1),f.Bq(2),f.Bq(3));
        fprintf(fid,'static const int16_t %s_FMAC_A[2] = {%d,%d};\n', ...
            n,f.Aq(1),f.Aq(2));
        fprintf(fid,'static const uint8_t %s_FMAC_R = %d;\n\n',n,f.R);
    end
end
