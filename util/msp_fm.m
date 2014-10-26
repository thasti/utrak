% MSP430 FM 

% input parameters
fdco = 8000000/8;
baud_target = 1200;
f1_target = 1200;
f2_target = 2200;
phase_res_bits = 10;

% possible sample frequency range
f_start = 8000;
f_end = 14000;

% calculation
N_start = round(fdco / f_start);
N_end = round(fdco / f_end);
M = 2^phase_res_bits;

res = zeros((f_start-f_end), 11);
i = 0;
for N = N_end:N_start
    i = i + 1;
    fs = fdco / N;      % resulting sampling frequency
    df = fs / M;        % minimum frequency step
    
    % frequency error calculation
    fcw1 = round(f1_target / df);
    f1 = fcw1*df;
    e_f1_perc = ((f1_target - f1) / f1_target) * 100;
    
    fcw2 = round(f2_target / df);
    f2 = fcw2*df;
    e_f2_perc = ((f2_target - f2) / f2_target) * 100;
    
    % baud rate error calculation
    samp_per_bit = round(fs / baud_target);
    baud = fs / samp_per_bit;
    e_baud_percent = ((baud_target - baud) / baud_target) * 100;
    
    res(i,:) = [ ...
        fs ...
        N ...
        baud ...
        e_baud_percent ...
        samp_per_bit ...
        fcw1 ...
        f1 ...
        e_f1_perc ...
        fcw2 ...
        f2 ...
        e_f2_perc];
end


hold on
grid on
stem(res(:,2), res(:,4));
stem(res(:,2), res(:,8),'r');
stem(res(:,2), res(:,11),'g');


