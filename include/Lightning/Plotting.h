/*
Created by Nathaniel Rupprecht on 3/16/24.

MIT License

Copyright (c) 2023 Nathaniel Rupprecht

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <filesystem>
#include "Lightning/Lightning.h"

namespace lightning {

//! \brief A class that represents a buildable figure. How the figure actually stores information or produces (or does
//! not produce) a figure is up to the implementation.
class Figure {
 public:
  virtual ~Figure() = default;

  //! \brief Set the x axis label of the figure.
  virtual void SetXLabel(const std::string &x_label) = 0;

  //! \brief Set the y axis label of the figure.
  virtual void SetYLabel(const std::string &x_label) = 0;

  //! \brief Set the title of the figure.
  virtual void SetTitle(const std::string &x_label) = 0;

  //! \brief Notifies the figure to plot the given y data vs the x data, potentially setting a plot label for the data.
  void Plot(const std::vector<double> &x, const std::vector<double> &y, std::string_view label = {}) {
    plot(x, y, label);
  }

  //! \brief Notifies the figure to scatter plot the given y data vs the x data, potentially setting a plot label for
  //! the data.
  void Scatter(const std::vector<double> &x, const std::vector<double> &y, std::string_view label = {}) {
    scatter(x, y, label);
  }

  //! \brief Notifies the figure to error bar plot the given y data vs the x data, potentially setting a plot label for
  //! the data.
  void ErrorBars(const std::vector<double> &x,
                 const std::vector<double> &y,
                 const std::vector<double> &y_err,
                 std::string_view label = {}) {
    errorBars(x, y, y_err, label);
  }

  virtual void SaveFigure(std::string_view filename) = 0;

  //! \brief Set a named option of type string.
  //!
  //! This is assumed to replace any option of the same name that currently exists, and last until either replaced, or
  //! ResetOptions is called. What options do any which are supported is implementation defined.
  virtual void AddOption(std::string_view label, std::string_view value) = 0;

  //! \brief Set a named option of type integer.
  //!
  //! This is assumed to replace any option of the same name that currently exists, and last until either replaced, or
  //! ResetOptions is called. What options do any which are supported is implementation defined.
  virtual void AddOption(std::string_view label, int value) = 0;

  //! \brief Set a named option of type double.
  //!
  //! This is assumed to replace any option of the same name that currently exists, and last until either replaced, or
  //! ResetOptions is called. What options do any which are supported is implementation defined.
  virtual void AddOption(std::string_view label, double value) = 0;

  //! \brief Remove all options.
  virtual void ResetOptions() = 0;

 private:
  virtual void plot(const std::vector<double> &x, const std::vector<double> &y, std::string_view label) = 0;
  virtual void scatter(const std::vector<double> &x, const std::vector<double> &y, std::string_view label) = 0;
  virtual void errorBars(const std::vector<double> &x,
                         const std::vector<double> &y,
                         const std::vector<double> &y_err,
                         std::string_view label) = 0;
};

//! \brief A class that represents a figure that serializes the data to a file. The file is designed to be read by an
//! accompanying Python script that will produce the figure using matplotlib's pyplot.
class MatplotlibSerializingFigure final : public Figure {
 public:
  //! \brief Construct a new MatplotlibSerializingFuture object with the specified image width and height, and the
  //! specified write directory where the image's data file representation will be saved.
  explicit MatplotlibSerializingFigure(double width, double height, std::filesystem::path write_directory)
      : write_directory_(std::move(write_directory)), width_(width), height_(height) {}

  void SetXLabel(const std::string &x_label) override { x_label_ = x_label; }
  void SetYLabel(const std::string &y_label) override { y_label_ = y_label; }
  void SetTitle(const std::string &title) override { title_ = title; }

  void SaveFigure(std::string_view local_file_path) override {
    // Create a data file name.
    std::string data_file_name{local_file_path};
    std::replace(data_file_name.begin(), data_file_name.end(), '.', '_');
    data_file_name += ".img";

    std::ofstream file(write_directory_ / data_file_name);
    if (file.is_open()) {
      ToStream(file, local_file_path);
    }
  }

  void AddOption(std::string_view label, std::string_view value) override {
    stream_ << 'O' << label << '\0' << 'S' << value << '\0';
  }

  void AddOption(std::string_view label, int value) override {
    stream_ << 'O' << label << '\0' << 'I';
    stream_.write(reinterpret_cast<const char*>(&value), sizeof(value));
  }

  void AddOption(std::string_view label, double value) override {
    stream_ << 'O' << label << '\0' << 'D';
    stream_.write(reinterpret_cast<const char*>(&value), sizeof(value));
  }

  void ResetOptions() override {
    stream_ << 'R';
  }

  void ToStream(std::ostream &stream, std::string_view save_path) const {
    stream << 's' << save_path << '\0';
    stream << "D";
    stream.write(reinterpret_cast<const char *>(&width_), sizeof(width_));
    stream.write(reinterpret_cast<const char *>(&height_), sizeof(height_));
    if (!x_label_.empty())
      stream << 'X' << x_label_ << '\0';
    if (!y_label_.empty())
      stream << 'Y' << y_label_ << '\0';
    if (!title_.empty())
      stream << 'T' << title_ << '\0';
    stream << stream_.str();
  }

  // ================================================================================================================
  //  Accessors
  // ================================================================================================================

  //! \brief The width of the figure.
  double GetWidth() const noexcept { return width_; }

  //! \brief The height of the figure.
  double GetHeight() const noexcept { return height_; }

  //! \brief The directory to which any plot data files (generated by calls to SaveFigure) should be written.
  const std::filesystem::path &GetWriteDirectory() const noexcept { return write_directory_; }

  //! \brief The title of the figure.
  std::string_view GetTitle() const noexcept { return title_; }

  //! \brief The x axis label of the figure.
  std::string_view GetXLabel() const noexcept { return x_label_; }

  //! \brief The y axis label of the figure.
  std::string_view GetYLabel() const noexcept { return y_label_; }

 private:
  void plot(const std::vector<double> &x, const std::vector<double> &y, std::string_view label) override {
    if (x.size() != y.size()) {
      return;
    }
    auto size = x.size();
    auto data_size = static_cast<std::streamsize>(x.size() * sizeof(double));
    stream_ << 'P';
    // Write the amount of data to expect.
    stream_.write(reinterpret_cast<const char *>(&size), sizeof(size));
    // Write the X data.
    stream_.write(reinterpret_cast<const char *>(x.data()), data_size);
    // Write the Y data.
    stream_.write(reinterpret_cast<const char *>(y.data()), data_size);
    // Write the label.
    stream_ << label << '\0';
  }

  void scatter(const std::vector<double> &x, const std::vector<double> &y, std::string_view label) override {
    if (x.size() != y.size()) {
      return;
    }
    auto size = x.size();
    auto data_size = static_cast<std::streamsize>(x.size() * sizeof(double));
    stream_ << 'S';
    // Write the amount of data to expect.
    stream_.write(reinterpret_cast<const char *>(&size), sizeof(size));
    // Write the X data.
    stream_.write(reinterpret_cast<const char *>(x.data()), data_size);
    // Write the Y data.
    stream_.write(reinterpret_cast<const char *>(y.data()), data_size);
    // Write the label.
    stream_ << label << '\0';
  }

  void errorBars(const std::vector<double> &x,
                 const std::vector<double> &y,
                 const std::vector<double> &y_err,
                 std::string_view label) override {
    if (x.size() != y.size() || y.size() != y_err.size()) {
      return;
    }
    auto size = static_cast<uint64_t>(x.size());
    auto data_size = static_cast<std::streamsize>(x.size() * sizeof(double));
    stream_ << 'E';
    // Write the amount of data to expect.
    stream_.write(reinterpret_cast<const char *>(&size), sizeof(size));
    // Write the X data.
    stream_.write(reinterpret_cast<const char *>(x.data()), data_size);
    // Write the Y data.
    stream_.write(reinterpret_cast<const char *>(y.data()), data_size);
    // Write the err data.
    stream_.write(reinterpret_cast<const char *>(y_err.data()), data_size);
    // Write the label.
    stream_ << label << '\0';
  }

  //! \brief The directory to which any plot data files (generated by calls to SaveFigure) should be written.
  std::filesystem::path write_directory_{};

  std::ostringstream stream_{};

  //! \brief The title of the figure.
  std::string title_{};

  //! \brief The x axis label of the figure.
  std::string x_label_{};

  //! \brief The y axis label of the figure.
  std::string y_label_{};

  //! \brief The width of the figure.
  double width_{};

  //! \brief The height of the figure.
  double height_{};
};

}  // namespace lightning