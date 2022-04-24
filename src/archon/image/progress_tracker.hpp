// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ARCHON_X_IMAGE_X_PROGRESS_TRACKER_HPP
#define ARCHON_X_IMAGE_X_PROGRESS_TRACKER_HPP

/// \file


#include <archon/image/image.hpp>


namespace archon::image {


/// \brief Handler for progress notifications.
///
/// This class is a specification of the interface of a progress notification
/// handler. Applications will hve to inherit from it, and override the \ref pgrogress()
/// function.
///
/// Instances of this class can be used with \ref image::try_load() and \ref
/// image::try_save() to track the progress of the loading or saving process.
///
class ProgressTracker {
public:
    /// \brief Inform about progress.
    ///
    /// This function is called repeatedly during the loading of, or saving of an image to
    /// inform the application about progress.
    ///
    /// Progress is measuren in number of units (\p units_completed). A unit is some
    /// arbitrary amount of work set by the implementation. It may change from one image to
    /// the next for the same file format.
    ///
    /// The progress, as a fraction, can be computed by dividing \p units_completed by \p
    /// units_total after conversion to floating point type. The total number of units (\p
    /// units_total) will always be greater than zero, and \p units_complete will always be
    /// less than, or equal to \p units_total. The application should expect \p units_total
    /// to generally remain unchanged across notifications, but constancy is not guaranteed.
    ///
    /// The fraction, computed as suggested above, is supposed to increase from one
    /// notification to the next, or at least not decrease. It is, however, allowed to
    /// decrease, so the application must be prepared for it.
    ///
    /// In any case, there will be a final invocation of this function where \p
    /// completed_states is equal to \p units_total.
    ///
    virtual void progress(const image::Image&, int units_completed, int units_total) = 0;

    virtual ~ProgressTracker() noexcept = default;
};


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_PROGRESS_TRACKER_HPP
