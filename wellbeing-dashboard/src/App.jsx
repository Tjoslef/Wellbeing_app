import { useEffect, useState } from "react";
import Mychart from "./assets/ApexLineChart";

function App() {
  const [data, setData] = useState({ history: [] });
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState(null);

  useEffect(() => {
    const fetch_data = () => {
      fetch("http://localhost:8080/logs")
        .then((res) => {
          if (!res.ok) throw new Error("Server neodpovídá");
          return res.json();
        })
        .then((jsonData) => {
          setData(jsonData);
          setIsLoading(false);
          setError(null);
        })
        .catch((err) => {
          console.error("Chyba:", err);
          setError(err.message);
          setIsLoading(false);
        });
    };

    fetch_data();
    const interval = setInterval(fetch_data, 5000);
    return () => clearInterval(interval);
  }, []);

  const history = data.history || [];
  const todayData = history.length > 1 ? history[0] : { logs: [], date: "" };
  const historyData = history.length > 1 ? history.slice(1, 6) : [];
  const chartCategories = (todayData.logs || []).map((item) => item.id);
  const chartSeries = [
    {
      name: "Čas v minutách",
      data: (todayData.logs || []).map((item) => Number(item.time_spend) || 0),
    },
  ];
  const historyPanelData = historyData.map((day) => ({
    date: day.date,
    categories: (day.logs || []).map((l) => l.id),
    series: [
      {
        name: "Minuty",
        data: (day.logs || []).map((l) => Number(l.time_spend) || 0),
      },
    ],
    topLogs: (day.logs || []).slice(0, 3),
  }));
  return (
    <div className="min-h-screen bg-zinc-100 p-8">
      <h1 className="mb-4 text-2xl font-bold text-zinc-800">
        Využití aplikací
      </h1>

      {isLoading ? (
        <div className="flex flex-col items-center justify-center rounded-xl bg-white p-10 shadow-sm">
          <div className="mb-4 h-10 w-10 animate-spin rounded-full border-b-2 border-indigo-600" />
          <p className="text-gray-500">Načítám data z C++ serveru...</p>
        </div>
      ) : error ? (
        <div className="rounded-lg bg-red-100 p-4 text-red-700 shadow-sm">
          Chyba: {error}. Běží váš C++ server na portu 8080?
        </div>
      ) : (
        <div className="space-y-8">
          {/* První graf: dnešní data */}
          <div className="rounded-xl bg-white p-4 shadow-sm">
            {todayData.logs.length > 0 ? (
              <Mychart
                categories={chartCategories}
                series={chartSeries}
                title="Dnešní aktivita (minuty)"
                type="bar"
              />
            ) : (
              <p className="py-10 text-center text-gray-500">
                Dnes zatím žádná data.
              </p>
            )}
          </div>

          {/* Druhý graf: historie */}
          <div className="mt-12">
            <h2 className="mb-4 text-xl font-bold">
              Historie posledních 7 dní
            </h2>

            {/* Horizontální scroll */}
            <div className="flex space-x-6 overflow-x-auto pb-4 scrollbar-hide">
              {historyPanelData.map((day) => (
                <div
                  key={day.date}
                  className="min-w-[320px] rounded-xl border border-zinc-200 bg-white p-4 shadow-sm"
                >
                  <h3 className="mb-2 font-bold text-indigo-600">{day.date}</h3>

                  <Mychart
                    height={200}
                    categories={day.categories}
                    series={day.series}
                    title=""
                    type="bar"
                  />

                  <div className="mt-4 border-t pt-2 text-sm text-gray-600">
                    {day.topLogs.map((log) => (
                      <div key={log.id} className="flex justify-between py-1">
                        <span className="truncate pr-4">{log.id}</span>
                        <span className="font-mono">{log.time_spend}m</span>
                      </div>
                    ))}
                  </div>
                </div>
              ))}
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

export default App;
