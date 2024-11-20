%% Digitalizar Señales
clear all
close all
%%  Pedimos al psoc el primer eco que usaremos como referencia, se abre el puerto serial:
%Longitud del vector de datos a recibir
longitud = 512;
%Frecuencia de muestreo
fs=0.4e6;
Ts=1/fs;
%Se configura el puerto serial y se abre el canal
delete(instrfind);
SerialPort='COM3'; %serial port
fincad = 'CR/LF';
baudios = 115200;
s = serial(SerialPort);
set(s,'BaudRate',baudios,'DataBits', 8, 'Parity', 'none','StopBits', 1,'FlowControl', 'none','Timeout',1);
set(s,'Terminator',fincad);
flushinput(s);
s.BytesAvailableFcnCount = longitud;
s.BytesAvailableFcnMode = 'byte';
%Se abre el puerto de comunicación
fopen(s);
%% Initializing variables - Primer grafico - Eco de la senhal de referencia
flag = 1;
flushinput(s);
           %% Se pide al psoc una nueva medicion
           MaxDeviation = 3;%Maximum Allowable Change from one value to next
           TimeInterval=0.001;%time interval between each input.
           tiempo = 0;
           senal_original = 0;
           %% Initializing variables - Primer grafico -
           flushinput(s);
           pause(1)
           fwrite(s,'I') %se pide al psoc que tome una medida de la señal
           pause(3)
           fwrite(s,'P')%se pide al psoc que envie por el puerto uart la medida tomada
           longitud = 512;
           senal_original(1)=0;
           tiempo(1)=0;
           count = 1;
           k=0;
           while ~isequal(count,longitud)
            %%Re creating Serial port before timeout 
               k=k+1; 
               if k==longitud
                   fclose(s);
                   delete(s);
                   clear s;       
                   s = serial(SerialPort);
                   set(s,'Terminator',fincad);
                   set(s,'BaudRate',baudios,'Parity','none');
                   fopen(s)    
                   k=0;
               end
           senal_original(count) = str2double(fscanf(s));
           tiempo(count) = count;
           count = count+1;
           end  
   figure
   title("Señal Filtrada")
   plot(senal_original)
  
%% Initializing variables - SEGUNDO GRAFICO - Eco de la senhal de referencia
flag = 1;
flushinput(s);
           %% Se pide al psoc una nueva medicion
           MaxDeviation = 3;%Maximum Allowable Change from one value to next
           TimeInterval=0.001;%time interval between each input.
           tiempo = 0;
           senal_filtrada = 0;
           %% Initializing variables - Primer grafico -
           flushinput(s);
           pause(1)
           fwrite(s,'F')%se pide al psoc que envie por el puerto uart la medida filtrada
           longitud = 512;
           senal_filtrada(1)=0;
           tiempo(1)=0;
           count = 1;
           k=0;
           while ~isequal(count,longitud)
            %%Re creating Serial port before timeout 
               k=k+1; 
               if k==longitud
                   fclose(s);
                   delete(s);
                   clear s;       
                   s = serial(SerialPort);
                   set(s,'Terminator',fincad);
                   set(s,'BaudRate',baudios,'Parity','none');
                   fopen(s)    
                   k=0;
               end
           senal_filtrada(count) = str2double(fscanf(s));
           tiempo(count) = count;
           count = count+1;
           end
   figure
   plot(senal_filtrada)
   title("Señal Filtrada")
  
%% Clean up the serial port
flushinput(s);
fclose(s);
delete(s);
clear s;
disp("Puerto serial cerrado")
%% Diseño del filtro
w1 = 0.2*pi; %Frec elegida que pasa
w2 = 0.4*pi;  %Frec que no tiene que pasar
A = [1 2*cos(w1); 1 2*cos(w2)]
B = [1;0]
x = A\B
alpha = x(1,1);
beta = x(2,1);
h = [alpha beta alpha];
figure
freqz(h,1)
title('Respuesta en frec');

