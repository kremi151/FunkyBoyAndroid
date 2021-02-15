package lu.kremi151.funkyboy.adapter

import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.BaseAdapter
import android.widget.ImageView
import android.widget.TextView
import androidx.annotation.DrawableRes
import androidx.annotation.IdRes
import androidx.annotation.StringRes
import lu.kremi151.funkyboy.R

class MainTilesAdapter(
        private val entries: List<Entry>,
        context: Context,
): BaseAdapter() {

    private val layoutInflater = LayoutInflater.from(context)

    override fun getCount(): Int = entries.size

    override fun getItem(position: Int): Any = entries[position]

    override fun getItemId(position: Int): Long = entries[position].id.toLong()
    fun getEntryId(position: Int): Int = entries[position].id

    override fun getView(position: Int, convertView: View?, parent: ViewGroup?): View {
        val viewHolder: ViewHolder
        val view: View = if (convertView != null) {
            viewHolder = convertView.tag as ViewHolder
            convertView
        } else {
            val view = layoutInflater.inflate(R.layout.tile_option, parent, false)
            viewHolder = ViewHolder(
                    view.findViewById(R.id.imageView),
                    view.findViewById(R.id.textView),
            )
            view.tag = viewHolder
            view
        }
        val entry = entries[position]
        viewHolder.apply {
            imageView.setImageResource(entry.iconRes)
            textView.setText(entry.titleRes)
        }
        return view
    }

    override fun isEnabled(position: Int) = entries[position].isEnabled()

    internal class ViewHolder (
            val imageView: ImageView,
            val textView: TextView,
    )

    class Entry (
            @IdRes val id: Int,
            @DrawableRes val iconRes: Int,
            @StringRes val titleRes: Int,
            val isEnabled: () -> Boolean,
    )

}