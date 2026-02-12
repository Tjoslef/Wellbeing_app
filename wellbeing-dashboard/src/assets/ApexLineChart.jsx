import Chart from "react-apexcharts";

const Mychart = ({ categories, series, title, type = "bar" }) => {
  if (!series || !series[0] || !series[0].data || series[0].data.length === 0) {
    return (
      <div className="h-[350px] flex items-center justify-center bg-white rounded-xl shadow-sm">
        Načítám graf...
      </div>
    );
  }

  const options = {
    chart: {
      id: title.replace(/\s+/g, "-").toLowerCase(),
      toolbar: { show: false },
      animations: {
        enabled: true,
        dynamicAnimation: { enabled: false },
      },
    },
    plotOptions: {
      bar: { borderRadius: 4, columnWidth: "45%" },
    },
    xaxis: {
      categories: categories,
    },
    title: {
      text: title,
      align: "left",
    },
    colors: ["#6366f1"],
  };

  return (
    <div className="bg-white p-4 rounded-xl shadow-sm">
      <Chart options={options} series={series} type={type} height={350} />
    </div>
  );
};

export default Mychart;
