namespace TitanExplorer
{
    partial class mainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(mainForm));
            this.statusStrip = new DarkUI.Controls.DarkStatusStrip();
            this.connectToolStripSplitButton = new System.Windows.Forms.ToolStripSplitButton();
            this.baudToolStripDropDownButton = new System.Windows.Forms.ToolStripDropDownButton();
            this.baudToolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
            this.baudToolStripMenuItem2 = new System.Windows.Forms.ToolStripMenuItem();
            this.baudToolStripMenuItem3 = new System.Windows.Forms.ToolStripMenuItem();
            this.baudToolStripMenuItem4 = new System.Windows.Forms.ToolStripMenuItem();
            this.bandwidthToolStripProgressBar = new System.Windows.Forms.ToolStripProgressBar();
            this.speedToolStripStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.logToolStripStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.serialPort = new System.IO.Ports.SerialPort(this.components);
            this.plotCheckedListBox = new System.Windows.Forms.CheckedListBox();
            this.panel1 = new System.Windows.Forms.Panel();
            this.plotCheckedListBoxEditTextbox = new DarkUI.Controls.DarkTextBox();
            this.closePlotsButton = new DarkUI.Controls.DarkButton();
            this.addPlotButton = new DarkUI.Controls.DarkButton();
            this.mainMenuStrip = new DarkUI.Controls.DarkMenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.alwaysOnTopToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripSeparator();
            this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.windowToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.logToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.graphWindowNoneToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.helpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.bindPanel = new System.Windows.Forms.Panel();
            this.bindLabel1 = new DarkUI.Controls.DarkLabel();
            this.bindGraphCheckBox = new DarkUI.Controls.DarkCheckBox();
            this.bindHereCheckBox = new DarkUI.Controls.DarkCheckBox();
            this.bindLogCheckBox = new DarkUI.Controls.DarkCheckBox();
            this.bindLogTextBox = new DarkUI.Controls.DarkTextBox();
            this.bindTextBox = new DarkUI.Controls.DarkTextBox();
            this.button1 = new DarkUI.Controls.DarkButton();
            this.button2 = new DarkUI.Controls.DarkButton();
            this.connectionTimer = new System.Windows.Forms.Timer(this.components);
            this.statusStrip.SuspendLayout();
            this.panel1.SuspendLayout();
            this.mainMenuStrip.SuspendLayout();
            this.bindPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // statusStrip
            // 
            this.statusStrip.AutoSize = false;
            this.statusStrip.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(60)))), ((int)(((byte)(63)))), ((int)(((byte)(65)))));
            this.statusStrip.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.connectToolStripSplitButton,
            this.baudToolStripDropDownButton,
            this.bandwidthToolStripProgressBar,
            this.speedToolStripStatusLabel,
            this.logToolStripStatusLabel});
            this.statusStrip.Location = new System.Drawing.Point(0, 422);
            this.statusStrip.Name = "statusStrip";
            this.statusStrip.Padding = new System.Windows.Forms.Padding(0, 5, 0, 3);
            this.statusStrip.Size = new System.Drawing.Size(800, 28);
            this.statusStrip.SizingGrip = false;
            this.statusStrip.TabIndex = 0;
            this.statusStrip.Text = "statusStrip";
            // 
            // connectToolStripSplitButton
            // 
            this.connectToolStripSplitButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.connectToolStripSplitButton.ForeColor = System.Drawing.Color.Red;
            this.connectToolStripSplitButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.connectToolStripSplitButton.Name = "connectToolStripSplitButton";
            this.connectToolStripSplitButton.Size = new System.Drawing.Size(67, 18);
            this.connectToolStripSplitButton.Text = "no ports";
            this.connectToolStripSplitButton.ButtonClick += new System.EventHandler(this.connectToolStripSplitButton_ButtonClick);
            this.connectToolStripSplitButton.DropDownOpening += new System.EventHandler(this.connectToolStripSplitButton_DropDownOpening);
            // 
            // baudToolStripDropDownButton
            // 
            this.baudToolStripDropDownButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.baudToolStripDropDownButton.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.baudToolStripMenuItem1,
            this.baudToolStripMenuItem2,
            this.baudToolStripMenuItem3,
            this.baudToolStripMenuItem4});
            this.baudToolStripDropDownButton.Image = ((System.Drawing.Image)(resources.GetObject("baudToolStripDropDownButton.Image")));
            this.baudToolStripDropDownButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.baudToolStripDropDownButton.Name = "baudToolStripDropDownButton";
            this.baudToolStripDropDownButton.ShowDropDownArrow = false;
            this.baudToolStripDropDownButton.Size = new System.Drawing.Size(47, 18);
            this.baudToolStripDropDownButton.Text = "115200";
            // 
            // baudToolStripMenuItem1
            // 
            this.baudToolStripMenuItem1.Name = "baudToolStripMenuItem1";
            this.baudToolStripMenuItem1.Size = new System.Drawing.Size(110, 22);
            this.baudToolStripMenuItem1.Text = "115200";
            this.baudToolStripMenuItem1.Click += new System.EventHandler(this.baudToolStripMenuItem_Click);
            // 
            // baudToolStripMenuItem2
            // 
            this.baudToolStripMenuItem2.Name = "baudToolStripMenuItem2";
            this.baudToolStripMenuItem2.Size = new System.Drawing.Size(110, 22);
            this.baudToolStripMenuItem2.Text = "230400";
            this.baudToolStripMenuItem2.Click += new System.EventHandler(this.baudToolStripMenuItem_Click);
            // 
            // baudToolStripMenuItem3
            // 
            this.baudToolStripMenuItem3.Name = "baudToolStripMenuItem3";
            this.baudToolStripMenuItem3.Size = new System.Drawing.Size(110, 22);
            this.baudToolStripMenuItem3.Text = "460800";
            this.baudToolStripMenuItem3.Click += new System.EventHandler(this.baudToolStripMenuItem_Click);
            // 
            // baudToolStripMenuItem4
            // 
            this.baudToolStripMenuItem4.Name = "baudToolStripMenuItem4";
            this.baudToolStripMenuItem4.Size = new System.Drawing.Size(110, 22);
            this.baudToolStripMenuItem4.Text = "921600";
            this.baudToolStripMenuItem4.Click += new System.EventHandler(this.baudToolStripMenuItem_Click);
            // 
            // bandwidthToolStripProgressBar
            // 
            this.bandwidthToolStripProgressBar.Name = "bandwidthToolStripProgressBar";
            this.bandwidthToolStripProgressBar.Size = new System.Drawing.Size(100, 14);
            // 
            // speedToolStripStatusLabel
            // 
            this.speedToolStripStatusLabel.Name = "speedToolStripStatusLabel";
            this.speedToolStripStatusLabel.Size = new System.Drawing.Size(33, 15);
            this.speedToolStripStatusLabel.Text = "0 B/s";
            // 
            // logToolStripStatusLabel
            // 
            this.logToolStripStatusLabel.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.logToolStripStatusLabel.Name = "logToolStripStatusLabel";
            this.logToolStripStatusLabel.Size = new System.Drawing.Size(551, 15);
            this.logToolStripStatusLabel.Spring = true;
            this.logToolStripStatusLabel.Text = "Log";
            this.logToolStripStatusLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // serialPort
            // 
            this.serialPort.DataReceived += new System.IO.Ports.SerialDataReceivedEventHandler(this.serialPort_DataReceived);
            // 
            // plotCheckedListBox
            // 
            this.plotCheckedListBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.plotCheckedListBox.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(60)))), ((int)(((byte)(63)))), ((int)(((byte)(65)))));
            this.plotCheckedListBox.CheckOnClick = true;
            this.plotCheckedListBox.ForeColor = System.Drawing.Color.Gainsboro;
            this.plotCheckedListBox.FormattingEnabled = true;
            this.plotCheckedListBox.Location = new System.Drawing.Point(0, 0);
            this.plotCheckedListBox.Name = "plotCheckedListBox";
            this.plotCheckedListBox.Size = new System.Drawing.Size(155, 334);
            this.plotCheckedListBox.TabIndex = 3;
            this.plotCheckedListBox.DoubleClick += new System.EventHandler(this.plotCheckedListBox_DoubleClick);
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.plotCheckedListBoxEditTextbox);
            this.panel1.Controls.Add(this.closePlotsButton);
            this.panel1.Controls.Add(this.addPlotButton);
            this.panel1.Controls.Add(this.plotCheckedListBox);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Right;
            this.panel1.Location = new System.Drawing.Point(645, 24);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(155, 398);
            this.panel1.TabIndex = 4;
            // 
            // plotCheckedListBoxEditTextbox
            // 
            this.plotCheckedListBoxEditTextbox.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(60)))), ((int)(((byte)(63)))), ((int)(((byte)(65)))));
            this.plotCheckedListBoxEditTextbox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.plotCheckedListBoxEditTextbox.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.plotCheckedListBoxEditTextbox.Location = new System.Drawing.Point(24, 27);
            this.plotCheckedListBoxEditTextbox.Name = "plotCheckedListBoxEditTextbox";
            this.plotCheckedListBoxEditTextbox.Size = new System.Drawing.Size(100, 20);
            this.plotCheckedListBoxEditTextbox.TabIndex = 6;
            this.plotCheckedListBoxEditTextbox.Visible = false;
            this.plotCheckedListBoxEditTextbox.Leave += new System.EventHandler(this.plotCheckedListBoxEditTextbox_Leave);
            // 
            // closePlotsButton
            // 
            this.closePlotsButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.closePlotsButton.Location = new System.Drawing.Point(74, 371);
            this.closePlotsButton.Name = "closePlotsButton";
            this.closePlotsButton.Padding = new System.Windows.Forms.Padding(5);
            this.closePlotsButton.Size = new System.Drawing.Size(78, 23);
            this.closePlotsButton.TabIndex = 5;
            this.closePlotsButton.Text = "Close Plots";
            this.closePlotsButton.Click += new System.EventHandler(this.closePlotsButton_Click);
            // 
            // addPlotButton
            // 
            this.addPlotButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.addPlotButton.Location = new System.Drawing.Point(3, 371);
            this.addPlotButton.Name = "addPlotButton";
            this.addPlotButton.Padding = new System.Windows.Forms.Padding(5);
            this.addPlotButton.Size = new System.Drawing.Size(62, 23);
            this.addPlotButton.TabIndex = 4;
            this.addPlotButton.Text = "Add Plot";
            this.addPlotButton.Click += new System.EventHandler(this.addPlotButton_Click);
            // 
            // mainMenuStrip
            // 
            this.mainMenuStrip.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(60)))), ((int)(((byte)(63)))), ((int)(((byte)(65)))));
            this.mainMenuStrip.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.mainMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.windowToolStripMenuItem,
            this.helpToolStripMenuItem});
            this.mainMenuStrip.Location = new System.Drawing.Point(0, 0);
            this.mainMenuStrip.Name = "mainMenuStrip";
            this.mainMenuStrip.Padding = new System.Windows.Forms.Padding(3, 2, 0, 2);
            this.mainMenuStrip.Size = new System.Drawing.Size(800, 24);
            this.mainMenuStrip.TabIndex = 5;
            this.mainMenuStrip.Text = "menuStrip";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(60)))), ((int)(((byte)(63)))), ((int)(((byte)(65)))));
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.alwaysOnTopToolStripMenuItem,
            this.toolStripMenuItem1,
            this.exitToolStripMenuItem});
            this.fileToolStripMenuItem.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.fileToolStripMenuItem.Text = "&File";
            // 
            // alwaysOnTopToolStripMenuItem
            // 
            this.alwaysOnTopToolStripMenuItem.CheckOnClick = true;
            this.alwaysOnTopToolStripMenuItem.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.alwaysOnTopToolStripMenuItem.Name = "alwaysOnTopToolStripMenuItem";
            this.alwaysOnTopToolStripMenuItem.Size = new System.Drawing.Size(149, 22);
            this.alwaysOnTopToolStripMenuItem.Text = "Always on top";
            this.alwaysOnTopToolStripMenuItem.CheckedChanged += new System.EventHandler(this.alwaysOnTopToolStripMenuItem_CheckedChanged);
            // 
            // toolStripMenuItem1
            // 
            this.toolStripMenuItem1.Name = "toolStripMenuItem1";
            this.toolStripMenuItem1.Size = new System.Drawing.Size(146, 6);
            // 
            // exitToolStripMenuItem
            // 
            this.exitToolStripMenuItem.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            this.exitToolStripMenuItem.Size = new System.Drawing.Size(149, 22);
            this.exitToolStripMenuItem.Text = "E&xit";
            this.exitToolStripMenuItem.Click += new System.EventHandler(this.exitToolStripMenuItem_Click);
            // 
            // windowToolStripMenuItem
            // 
            this.windowToolStripMenuItem.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(60)))), ((int)(((byte)(63)))), ((int)(((byte)(65)))));
            this.windowToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.logToolStripMenuItem,
            this.toolStripSeparator1,
            this.graphWindowNoneToolStripMenuItem});
            this.windowToolStripMenuItem.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.windowToolStripMenuItem.Name = "windowToolStripMenuItem";
            this.windowToolStripMenuItem.Size = new System.Drawing.Size(63, 20);
            this.windowToolStripMenuItem.Text = "&Window";
            // 
            // logToolStripMenuItem
            // 
            this.logToolStripMenuItem.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.logToolStripMenuItem.Name = "logToolStripMenuItem";
            this.logToolStripMenuItem.Size = new System.Drawing.Size(172, 22);
            this.logToolStripMenuItem.Text = "&Log";
            this.logToolStripMenuItem.Click += new System.EventHandler(this.logToolStripMenuItem_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(169, 6);
            // 
            // graphWindowNoneToolStripMenuItem
            // 
            this.graphWindowNoneToolStripMenuItem.Enabled = false;
            this.graphWindowNoneToolStripMenuItem.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(153)))), ((int)(((byte)(153)))), ((int)(((byte)(153)))));
            this.graphWindowNoneToolStripMenuItem.Name = "graphWindowNoneToolStripMenuItem";
            this.graphWindowNoneToolStripMenuItem.Size = new System.Drawing.Size(172, 22);
            this.graphWindowNoneToolStripMenuItem.Text = "No graphs opened";
            // 
            // helpToolStripMenuItem
            // 
            this.helpToolStripMenuItem.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(60)))), ((int)(((byte)(63)))), ((int)(((byte)(65)))));
            this.helpToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.aboutToolStripMenuItem});
            this.helpToolStripMenuItem.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.helpToolStripMenuItem.Name = "helpToolStripMenuItem";
            this.helpToolStripMenuItem.Size = new System.Drawing.Size(44, 20);
            this.helpToolStripMenuItem.Text = "&Help";
            // 
            // aboutToolStripMenuItem
            // 
            this.aboutToolStripMenuItem.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(60)))), ((int)(((byte)(63)))), ((int)(((byte)(65)))));
            this.aboutToolStripMenuItem.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.aboutToolStripMenuItem.Name = "aboutToolStripMenuItem";
            this.aboutToolStripMenuItem.Size = new System.Drawing.Size(107, 22);
            this.aboutToolStripMenuItem.Text = "&About";
            this.aboutToolStripMenuItem.Click += new System.EventHandler(this.aboutToolStripMenuItem_Click);
            // 
            // bindPanel
            // 
            this.bindPanel.Controls.Add(this.bindLabel1);
            this.bindPanel.Controls.Add(this.bindGraphCheckBox);
            this.bindPanel.Controls.Add(this.bindHereCheckBox);
            this.bindPanel.Controls.Add(this.bindLogCheckBox);
            this.bindPanel.Controls.Add(this.bindLogTextBox);
            this.bindPanel.Controls.Add(this.bindTextBox);
            this.bindPanel.Location = new System.Drawing.Point(27, 76);
            this.bindPanel.Name = "bindPanel";
            this.bindPanel.Size = new System.Drawing.Size(341, 103);
            this.bindPanel.TabIndex = 6;
            // 
            // bindLabel1
            // 
            this.bindLabel1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.bindLabel1.AutoSize = true;
            this.bindLabel1.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.bindLabel1.Location = new System.Drawing.Point(3, 12);
            this.bindLabel1.Name = "bindLabel1";
            this.bindLabel1.Size = new System.Drawing.Size(54, 13);
            this.bindLabel1.TabIndex = 11;
            this.bindLabel1.Text = "Bind from:";
            // 
            // bindGraphCheckBox
            // 
            this.bindGraphCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.bindGraphCheckBox.AutoSize = true;
            this.bindGraphCheckBox.Location = new System.Drawing.Point(168, 11);
            this.bindGraphCheckBox.Name = "bindGraphCheckBox";
            this.bindGraphCheckBox.Size = new System.Drawing.Size(55, 17);
            this.bindGraphCheckBox.TabIndex = 10;
            this.bindGraphCheckBox.Text = "Graph";
            // 
            // bindHereCheckBox
            // 
            this.bindHereCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.bindHereCheckBox.AutoSize = true;
            this.bindHereCheckBox.Checked = true;
            this.bindHereCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this.bindHereCheckBox.Enabled = false;
            this.bindHereCheckBox.Location = new System.Drawing.Point(63, 11);
            this.bindHereCheckBox.Name = "bindHereCheckBox";
            this.bindHereCheckBox.Size = new System.Drawing.Size(49, 17);
            this.bindHereCheckBox.TabIndex = 9;
            this.bindHereCheckBox.Text = "Here";
            // 
            // bindLogCheckBox
            // 
            this.bindLogCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.bindLogCheckBox.AutoSize = true;
            this.bindLogCheckBox.Location = new System.Drawing.Point(118, 11);
            this.bindLogCheckBox.Name = "bindLogCheckBox";
            this.bindLogCheckBox.Size = new System.Drawing.Size(44, 17);
            this.bindLogCheckBox.TabIndex = 8;
            this.bindLogCheckBox.Text = "Log";
            // 
            // bindLogTextBox
            // 
            this.bindLogTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.bindLogTextBox.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(69)))), ((int)(((byte)(73)))), ((int)(((byte)(74)))));
            this.bindLogTextBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.bindLogTextBox.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.bindLogTextBox.Location = new System.Drawing.Point(0, 34);
            this.bindLogTextBox.Name = "bindLogTextBox";
            this.bindLogTextBox.ReadOnly = true;
            this.bindLogTextBox.Size = new System.Drawing.Size(341, 20);
            this.bindLogTextBox.TabIndex = 7;
            // 
            // bindTextBox
            // 
            this.bindTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.bindTextBox.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(69)))), ((int)(((byte)(73)))), ((int)(((byte)(74)))));
            this.bindTextBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.bindTextBox.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(220)))));
            this.bindTextBox.Location = new System.Drawing.Point(0, 53);
            this.bindTextBox.Multiline = true;
            this.bindTextBox.Name = "bindTextBox";
            this.bindTextBox.Size = new System.Drawing.Size(341, 50);
            this.bindTextBox.TabIndex = 0;
            this.bindTextBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.bindTextBox_KeyDown);
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(52, 259);
            this.button1.Name = "button1";
            this.button1.Padding = new System.Windows.Forms.Padding(5);
            this.button1.Size = new System.Drawing.Size(121, 23);
            this.button1.TabIndex = 7;
            this.button1.Text = "Send Garbage 0";
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(179, 259);
            this.button2.Name = "button2";
            this.button2.Padding = new System.Windows.Forms.Padding(5);
            this.button2.Size = new System.Drawing.Size(126, 23);
            this.button2.TabIndex = 8;
            this.button2.Text = "Send Garbage 1";
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // connectionTimer
            // 
            this.connectionTimer.Interval = 1000;
            this.connectionTimer.Tick += new System.EventHandler(this.connectionTimer_Tick);
            // 
            // mainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 450);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.bindPanel);
            this.Controls.Add(this.panel1);
            this.Controls.Add(this.statusStrip);
            this.Controls.Add(this.mainMenuStrip);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MainMenuStrip = this.mainMenuStrip;
            this.Name = "mainForm";
            this.Text = "TITAN Explorer";
            this.Activated += new System.EventHandler(this.mainForm_Activated);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.mainForm_FormClosing);
            this.Load += new System.EventHandler(this.mainForm_Load);
            this.Resize += new System.EventHandler(this.mainForm_Resize);
            this.statusStrip.ResumeLayout(false);
            this.statusStrip.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.mainMenuStrip.ResumeLayout(false);
            this.mainMenuStrip.PerformLayout();
            this.bindPanel.ResumeLayout(false);
            this.bindPanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.ToolStripDropDownButton baudToolStripDropDownButton;
        private System.Windows.Forms.ToolStripStatusLabel logToolStripStatusLabel;
        private System.IO.Ports.SerialPort serialPort;
        private System.Windows.Forms.CheckedListBox plotCheckedListBox;
        private System.Windows.Forms.Panel panel1;
        private DarkUI.Controls.DarkButton closePlotsButton;
        private DarkUI.Controls.DarkButton addPlotButton;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem windowToolStripMenuItem;
        public System.Windows.Forms.ToolStripMenuItem logToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem graphWindowNoneToolStripMenuItem;
        private System.Windows.Forms.Panel bindPanel;
        private DarkUI.Controls.DarkButton button1;
        private DarkUI.Controls.DarkButton button2;
        private System.Windows.Forms.ToolStripMenuItem alwaysOnTopToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem1;
        private DarkUI.Controls.DarkMenuStrip mainMenuStrip;
        private DarkUI.Controls.DarkStatusStrip statusStrip;
        private DarkUI.Controls.DarkTextBox plotCheckedListBoxEditTextbox;
        private DarkUI.Controls.DarkTextBox bindTextBox;
        private DarkUI.Controls.DarkTextBox bindLogTextBox;
        private System.Windows.Forms.Timer connectionTimer;
        private System.Windows.Forms.ToolStripStatusLabel speedToolStripStatusLabel;
        private System.Windows.Forms.ToolStripMenuItem helpToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem;
        private System.Windows.Forms.ToolStripProgressBar bandwidthToolStripProgressBar;
        private DarkUI.Controls.DarkLabel bindLabel1;
        private DarkUI.Controls.DarkCheckBox bindGraphCheckBox;
        private DarkUI.Controls.DarkCheckBox bindHereCheckBox;
        private DarkUI.Controls.DarkCheckBox bindLogCheckBox;
        private System.Windows.Forms.ToolStripSplitButton connectToolStripSplitButton;
        private System.Windows.Forms.ToolStripMenuItem baudToolStripMenuItem1;
        private System.Windows.Forms.ToolStripMenuItem baudToolStripMenuItem2;
        private System.Windows.Forms.ToolStripMenuItem baudToolStripMenuItem3;
        private System.Windows.Forms.ToolStripMenuItem baudToolStripMenuItem4;
    }
}

