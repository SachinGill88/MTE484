%% Lab 3 Part C: emulation design by polynomial pole placement
clear; clc; close all;
format long;
s = tf('s');
Ts = 0.005;
K2 = 0.0609;
K3 = -4.28; % Provisional combined Lab 2 estimate; use the final report value.
b = K2*K3;

% Inner-loop settling is much faster than the outer-loop response.
% Approximate the inner closed loop as unity ONLY for controller synthesis.
% For a second-order approximation, Ts_2pct <= 7 implies sigma >= 4/7.
% Overshoot < 45% implies zeta > 0.2464.
% The pair below has sigma = 0.85 and zeta approximately 0.862.
% -0.1 supplies a slow bias-correction mode; -0.85 completes the fourth-order
% design on the transient timescale. The -0.1 pole is slower than the pair:
% the pair is NOT assumed dominant. All modes/zeros are verified below.
desiredPoles = [-0.85+0.5i, -0.85-0.5i, -0.1, -0.85];
q = real(poly(desiredPoles));

% C2(s) = (n2*s^2+n1*s+n0)/(s*(s+p)).
% With Papprox=b/s^2, the closed-loop polynomial is
% s^4+p*s^3+b*n2*s^2+b*n1*s+b*n0.
% Match every coefficient to q; no controller gains are fitted to plots.
p = q(2);
n = q(3:5)/b;
C2 = tf(n, [1 p 0]);
Tapprox = feedback(C2*b/s^2, 1);
disp('Desired poles:'); disp(desiredPoles.');
disp('Achieved approximate-model poles:'); disp(pole(Tapprox));
disp('Continuous outer controller:'); C2
assert(isstable(Tapprox), 'Approximate closed loop is unstable.');
assert(abs(dcgain(Tapprox)-1)<1e-8, 'Nonzero nominal step error.');

%% Verify with the complete inner loop, rather than the unity approximation
Pmotor = -2.114507/(s*(0.0216084*s+1));
C1 = (0.883306236418925*s-337.895190699298) ...
     /(s+72.7217008200515);
Tinner = feedback(C1*Pmotor, 1);
Tfull = feedback(C2*Tinner*b/s^2, 1);
disp('Full continuous closed-loop poles:'); disp(pole(Tfull));
assert(isstable(Tfull), 'Full continuous closed loop is unstable.');
disp('Full continuous unit-step metrics:'); disp(stepinfo(Tfull));

D1 = tf([0.032635065920414 -1.462208491947337], ...
        [1 -0.692327639311257], Ts);
D2 = c2d(C2, Ts, 'tustin');
disp('Tustin outer controller:'); D2
[numD2, denD2] = tfdata(D2, 'v');
disp('D2 numerator:'); disp(numD2);
disp('D2 denominator:'); disp(denD2);

%% Realize C2 as a lead branch plus an integrator for conditional integration
% C2 = n2 + kI/s + r/(s+p).
kI = n(3)/p;
Clead = tf([n(1), n(2)-kI], [1 p]);
Dlead = c2d(Clead, Ts, 'tustin');
[nl, dl] = tfdata(Dlead, 'v');
[ni, di] = tfdata(D1, 'v');

% Exact ZOH propagation of the connected physical motor/beam/ball plant.
% States: [gear angle; angular velocity; ball position; ball velocity].
A = [0 1 0 0; 0 -1/0.0216084 0 0; 0 0 0 1; b 0 0 0];
B = [0; -2.114507/0.0216084; 0; 0];
plantD = c2d(ss(A,B,eye(4),zeros(4,1)),Ts,'zoh');

% Nominal sampled linear stability check, without saturation.
% State-space connection avoids high-order polynomial cancellation near z=1.
innerSampled = feedback(plantD*ss(D1),[1 0 0 0]);
TfullD = feedback(innerSampled(3,1)*ss(D2),1);
assert(isstable(TfullD), 'Full sampled linear model is unstable.');

%% Simulate 0.10 <-> 0.25 m, 30 s period, with +/-0.7 rad saturation
t = (0:Ts:120).';
ref = 0.10 + 0.15*(mod(t,30)>=15);
position = zeros(size(t)); angle = position; thetaRaw = position;
voltage = position;
x = [0;0;0.10;0]; % Equilibrium at the initial reference; no release disturbance.
previousE = 0; previousLead = 0; integral = 0;
previousInnerE = 0; previousU = 0;
for k = 1:numel(t)
    e = ref(k)-x(3);
    lead = -dl(2)*previousLead + nl(1)*e + nl(2)*previousE;
    deltaI = kI*Ts/2*(e+previousE);
    candidate = lead+integral+deltaI;
    if ~(candidate>0.7 && deltaI>0) && ~(candidate< -0.7 && deltaI<0)
        integral = integral+deltaI;
    end
    thetaRaw(k) = lead+integral;
    thetaCommand = min(0.7,max(-0.7,thetaRaw(k)));
    innerE = thetaCommand-x(1);
    u = -di(2)*previousU + ni(1)*innerE + ni(2)*previousInnerE;
    position(k) = x(3); angle(k) = x(1); voltage(k) = u;
    x = plantD.A*x+plantD.B*u;
    previousE=e; previousLead=lead; previousInnerE=innerE; previousU=u;
end

% This nominal simulation has no friction, gravity bias, end stops, voltage
% clipping or safety shutdown. It assesses the model and reference saturation;
% it cannot establish hardware safety or performance.
stepTimes = (15:15:105).';
settling = nan(size(stepTimes)); overshoot = settling; tailError = settling;
for k=1:numel(stepTimes)
    ix=find(t>=stepTimes(k) & t<stepTimes(k)+15);
    target=ref(ix(1)); values=position(ix);
    direction=sign(target-ref(ix(1)-1));
    overshoot(k)=max(0,max(direction*(values-target)))/0.15*100;
    lastOutside=find(abs(values-target)>0.003,1,'last');
    if isempty(lastOutside)
        settling(k)=0;
    elseif lastOutside<numel(ix)
        settling(k)=t(ix(lastOutside+1))-stepTimes(k);
    end
    tailError(k)=target-mean(values(end-round(3/Ts)+1:end));
end
results=table(stepTimes,settling,overshoot,tailError, ...
    settling<=7,overshoot<45,'VariableNames', ...
    {'StepTime_s','Settling_s','Overshoot_percent','FinalWindowError_m', ...
     'SettlingPass','OvershootPass'});
disp(results);
fprintf('Max |requested gear reference| = %.6f rad\n',max(abs(thetaRaw)));
fprintf('Max |model motor voltage| = %.6f V\n',max(abs(voltage)));
fprintf('Max |model measured gear angle| = %.6f rad\n',max(abs(angle)));
fprintf('Nominal sampled DC gain: %.12f\n',dcgain(TfullD));

figure;
plot(t,ref,'k--',t,position,'b','LineWidth',1.2);
grid on; xlabel('Time (s)'); ylabel('Ball position (m)');
legend('Reference','Full sampled model','Location','best');
title('Lab 3 Part C: pole-placement controller with reference saturation');

% Export C2/D2 or enter these blocks in the full Simulink model for the report.
% Do not replace the Arduino controller until this model and chosen K3 are reviewed.
