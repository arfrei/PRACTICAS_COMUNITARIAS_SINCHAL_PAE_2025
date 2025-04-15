//SE CREA UN GRAFICO CON MATLAB VISUALIZACION EN THINGSPEAK
// SE TOMAN DE GRUPO DE 6 LECTURAS (1 min de datos) y se calcula el rango de fiabilidad mediante IQR
// LOS VALORES DENTRO DE ESE RANGO SE LES CALCULA LA MEDIA 
// LA MEDIA ES ES VALOR PUBLICADO EN EL GRAFICO

% Configuración del canal
readChannelID = ;
fieldID = 1;
readAPIKey = '';

% Leer los últimos 30 datos
numPoints = 30;
[data, time] = thingSpeakRead(readChannelID, 'Field', fieldID, 'NumPoints', numPoints, 'ReadKey', readAPIKey);

% Asegurar que haya suficientes datos
if length(data) < 6
    error('No hay suficientes datos para procesar.');
end

% Tamaño del grupo (6 lecturas por grupo)
groupSize = 6;
numGroups = floor(length(data) / groupSize);

% Inicializar vectores
filteredMeans = [];
filteredTimes = datetime.empty;  % ← Solución

% Procesar cada grupo
for i = 1:numGroups
    idxStart = (i - 1) * groupSize + 1;
    idxEnd = i * groupSize;

    groupData = data(idxStart:idxEnd);
    groupTime = time(idxStart:idxEnd);

    % Calcular IQR
    Q1 = prctile(groupData, 25);
    Q3 = prctile(groupData, 75);
    IQR = Q3 - Q1;

    lowerBound = Q1 - 1.5 * IQR;
    upperBound = Q3 + 1.5 * IQR;

    % Filtrar datos dentro de la franja
    validData = groupData(groupData >= lowerBound & groupData <= upperBound);

    if isempty(validData)
        filteredMean = NaN;  % Podrías también omitir este punto
    else
        filteredMean = mean(validData);
    end

    % Guardar resultados
    filteredMeans(end+1) = filteredMean;
    filteredTimes(end+1) = groupTime(end);  % usar el último timestamp del grupo
end

% Ajustar zona horaria (UTC-5)
filteredTimes = filteredTimes - hours(5);

% Calcular límites para el eje Y
minY = min(filteredMeans) - 4;
maxY = max(filteredMeans) + 4;

% Graficar línea + puntos
figure;
plot(filteredTimes, filteredMeans, 'o-', 'LineWidth', 1.5, ...
    'MarkerSize', 6, 'MarkerFaceColor', 'blue');
xlabel('Tiempo');
ylabel('Nivel Promedio (cm)');
ylim([minY, maxY]);
title('Nivel Promedio del Reservorio');
grid on;

% Agregar los valores como texto al lado de cada punto
for i = 1:length(filteredMeans)
    if ~isnan(filteredMeans(i))
        % Mostrar el número con un decimal
        label = sprintf('%.1f', filteredMeans(i));
        % Ajustar un poco la posición vertical para que no se encime
        text(filteredTimes(i), filteredMeans(i) + 0.8, label, ...
            'HorizontalAlignment', 'center', 'FontSize', 10, 'Color', 'blue');
    end
end
