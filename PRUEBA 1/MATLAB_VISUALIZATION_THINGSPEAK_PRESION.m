% Canal y campo de ThingSpeak
readChannelID = 2909722;
fieldID = 2;
readAPIKey = 'E281BGC064OGVMUL';

% Leer los últimos 120 puntos (~20 minutos si llegan cada 10 segundos)
[data, time] = thingSpeakRead(readChannelID, ...
    'Field', fieldID, ...
    'NumPoints', 120, ...
    'ReadKey', readAPIKey);

if isempty(data)
    error('No se pudieron leer datos del canal.');
end

% CORREGIR ZONA HORARIA (UTC-5)
time = time - hours(5);

%% Agrupación por cada 2 minutos (12 muestras por grupo)
samplesPerGroup = 12;
nGroups = floor(length(data) / samplesPerGroup);

modaData = zeros(nGroups, 1);
modaTime = datetime([], [], []);

for i = 1:nGroups
    idxStart = (i - 1) * samplesPerGroup + 1;
    idxEnd = i * samplesPerGroup;

    modaData(i) = mode(data(idxStart:idxEnd));
    modaTime(i) = time(idxEnd);  % Timestamp corregido
end

%% Visualización
figure;
plot(modaTime, modaData, '-o', 'MarkerSize', 6, 'MarkerFaceColor', 'b', 'Color', 'b');
grid on;

% Etiquetas de los puntos SIN decimales
for i = 1:length(modaData)
    text(modaTime(i), modaData(i) + 0.6, sprintf('%d', round(modaData(i))), ...
        'Color', 'blue', ...
        'FontSize', 10, ...
        'HorizontalAlignment', 'center');
end

% Títulos y ejes
title('Presión Promedio (MODA) de la Junta de Agua');
xlabel('Tiempo');
ylabel('Presión (psi)');
ylim([min(modaData) - 2, max(modaData) + 2]);
