using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using DarkUI.Forms;

namespace TitanExplorer
{
    public partial class graphForm : DarkForm
    {
        public int savedHeight;

        public int graph_id;
        public int max_points;
        private List<int> signal_id;
        private List<int> x_signal;

        Point? prevPosition = null;
        ToolTip tooltip = new ToolTip();
        int pointsWaiting = 0;

        public graphForm()
        {
            InitializeComponent();

            signal_id = new List<int>();
            x_signal = new List<int>();
            max_points = 100;

            chart.ChartAreas[0].AxisY.Minimum = Double.NaN;
            chart.ChartAreas[0].AxisY.Maximum = Double.NaN;
            chart.ChartAreas[0].AxisX.Minimum = Double.NaN;
            chart.ChartAreas[0].AxisX.Maximum = Double.NaN;

            maxPointsToolStripComboBox.SelectedIndex = 4;
        }

        private void graphForm_Resize(object sender, EventArgs e)
        {
            if (WindowState != FormWindowState.Minimized)
            {
                savedHeight = Height;
            }
        }

        public void Clear()
        {
            chart.ChartAreas[0].AxisY.Minimum = Double.NaN;
            chart.ChartAreas[0].AxisY.Maximum = Double.NaN;
            chart.ChartAreas[0].AxisX.Minimum = Double.NaN;
            chart.ChartAreas[0].AxisX.Maximum = Double.NaN;
            for (int i = 0; i < signal_id.Count; i++)
            {
                chart.Series[i].Points.Clear();
            }
        }

        public void addSignal(int id, string name)
        {
            chart.Series.Add(name);
            chart.Series[chart.Series.Count - 1].ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
            x_signal.Add(0);
            signal_id.Add(id);
        }

        public void addData(System.UInt32 channel, double y)
        {
            if (freezeToolStripMenuItem.Checked)
            {
                return;
            }

            for (int i = 0; i < signal_id.Count; i++)
            {
                if (channel == signal_id[i])
                {
                    while (chart.Series[i].Points.Count >= max_points)
                    {
                        chart.Series[i].Points.RemoveAt(0);
                    }

                    chart.Series[i].Points.AddXY(x_signal[i], y);
                    x_signal[i]++;
                    pointsWaiting++;

                    if(pointsWaiting > 10)
                    {
                        rescaleGraph(null, null, autoRescaleToolStripMenuItem.Checked);
                        timerRefresh.Enabled = false;
                    }
                    else
                    if (timerRefresh.Enabled == false)
                    {
                        timerRefresh.Enabled = true;
                    }
                }
            }
        }
        
        private void chart_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == ' ')
            {
                freezeToolStripMenuItem_Click(sender, null);
            }
            else
            if ((e.KeyChar == 'R') || (e.KeyChar == 'r'))
            {
                rescaleToolStripMenuItem_Click(sender, null);
            }
            else
            if ((e.KeyChar == 'S') || (e.KeyChar == 's'))
            {
                if (saveSnapshotToolStripMenuItem.Enabled)
                {
                    saveSnapshotToolStripMenuItem_Click(sender, null);
                }
            }
            else
            if ((e.KeyChar == 'C') || (e.KeyChar == 'c'))
            {
                if (saveCSVToolStripMenuItem.Enabled)
                {
                    saveCSVToolStripMenuItem_Click(sender, null);
                }
            }
        }

        private void maxPointsToolStripComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            max_points = Convert.ToInt32(maxPointsToolStripComboBox.Items[maxPointsToolStripComboBox.SelectedIndex]);
        }

        private void rescaleGraph(object sender, EventArgs e, bool rescale_y)
        {
            double maxX = Double.MinValue;
            double minX = Double.MaxValue;
            double maxY = Double.MinValue;
            double minY = Double.MaxValue;

            for (int i = 0; i < chart.Series.Count; i++)
            {
                foreach (DataPoint dp in chart.Series[i].Points)
                {
                    minX = Math.Min(minX, dp.XValue);
                    maxX = Math.Max(maxX, dp.XValue);

                    minY = Math.Min(minY, dp.YValues[0]);
                    maxY = Math.Max(maxY, dp.YValues[0]);
                }
            }

            chart.ChartAreas[0].AxisX.Maximum = maxX;
            chart.ChartAreas[0].AxisX.Minimum = minX;
            pointsWaiting = 0;

            if (rescale_y)
            {
                double del = 0.02 * (maxY - minY);
                chart.ChartAreas[0].AxisY.Maximum = maxY + del;
                chart.ChartAreas[0].AxisY.Minimum = minY - del;
            }
        }

        private void rescaleToolStripMenuItem_Click(object sender, EventArgs e)
        {
            rescaleGraph(sender, e, true);
        }

        private void autoSizeYToolStripMenuItem_Click(object sender, EventArgs e)
        {
            rescaleToolStripMenuItem.Enabled = autoRescaleToolStripMenuItem.Checked;
            autoRescaleToolStripMenuItem.Checked = !autoRescaleToolStripMenuItem.Checked;
        }

        private void freezeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            freezeToolStripMenuItem.Checked = !freezeToolStripMenuItem.Checked;
            if(freezeToolStripMenuItem.Checked == false)
            {
                Clear();
            }
            saveSnapshotToolStripMenuItem.Enabled = freezeToolStripMenuItem.Checked;
            saveCSVToolStripMenuItem.Enabled = freezeToolStripMenuItem.Checked;
        }

        private void saveSnapshotToolStripMenuItem_Click(object sender, EventArgs e)
        {
            saveFileDialog.Filter = "PNG files|*.png";
            if (saveFileDialog.ShowDialog() == DialogResult.OK)
            {
                chart.SaveImage(saveFileDialog.FileName, ChartImageFormat.Png);
            }
        }

        private void saveCSVToolStripMenuItem_Click(object sender, EventArgs e)
        {
            saveFileDialog.Filter = "CSV files|*.csv";
            if (saveFileDialog.ShowDialog() == DialogResult.OK)
            {
                var csv = new StringBuilder();
                var header = "\"Sample\"";

                for (int i = 0; i < chart.Series.Count; i++)
                {
                    header += ",\"" + chart.Series[i].Name + "\"";
                }
                csv.AppendLine(header);

                for(int i = 0; i < chart.Series[0].Points.Count; i++)
                {
                    var s = chart.Series[0].Points[i].XValue.ToString();
                    for (int j = 0; j < chart.Series.Count; j++)
                    {
                        s += "," + chart.Series[j].Points[i].YValues[0].ToString();
                    }
                    csv.AppendLine(s);
                }

                File.WriteAllText(saveFileDialog.FileName, csv.ToString());
            }
        }

        private void closeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void timerRefresh_Tick(object sender, EventArgs e)
        {
            rescaleGraph(sender, e, autoRescaleToolStripMenuItem.Checked);
            timerRefresh.Enabled = false;
        }

        private void alwaysOnTopToolStripMenuItem_CheckedChanged(object sender, EventArgs e)
        {
            TopMost = alwaysOnTopToolStripMenuItem.Checked;
        }

        private void chart_MouseMove(object sender, MouseEventArgs e)
        {
            var pos = e.Location;
            if (prevPosition.HasValue && pos == prevPosition.Value)
            {
                return;
            }
            tooltip.RemoveAll();
            prevPosition = pos;
            var results = chart.HitTest(pos.X, pos.Y, false,
                                            ChartElementType.DataPoint);
            foreach (var result in results)
            {
                if (result.ChartElementType == ChartElementType.DataPoint)
                {
                    var prop = result.Object as DataPoint;
                    if (prop != null)
                    {
                        var pointXPixel = result.ChartArea.AxisX.ValueToPixelPosition(prop.XValue);
                        var pointYPixel = result.ChartArea.AxisY.ValueToPixelPosition(prop.YValues[0]);

                        // check if the cursor is really close to the point (2 pixels around the point)
                        if (Math.Abs(pos.X - pointXPixel) < 10 && Math.Abs(pos.Y - pointYPixel) < 10)
                        {
                            tooltip.Show("X=" + prop.XValue + ", Y=" + prop.YValues[0], this.chart, pos.X, pos.Y - 15);
                        }
                    }
                }
            }
        }
    }
}
