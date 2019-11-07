package com.buho29.jardin.utils;

import java.util.LinkedList;

public class EnQueue<E> {
	private LinkedList<E> list = new LinkedList<E>();

	public void enqueue(E item) {
		list.addLast(item);
	}

	public E dequeue() {
		return list.poll();
	}

	public boolean hasItems() {
		return !list.isEmpty();
	}

	public int size() {
		return list.size();
	}

	public void addItems(EnQueue<? extends E> q) {
		while (q.hasItems())
			list.addLast(q.dequeue());
	}


	public void clear() {
		list.clear();
	}

	public boolean contain(E item) {
		return list.contains(item);
	}
}
